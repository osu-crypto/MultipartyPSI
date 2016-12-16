#include "OPPRFReceiver.h"
#include <future>

#include "Crypto/PRNG.h"
#include "Crypto/Commit.h"

#include "Common/Log.h"
#include "Common/Log1.h"
#include "Base/naor-pinkas.h"
#include <unordered_map>

#include "TwoChooseOne/IknpOtExtReceiver.h"
#include "TwoChooseOne/IknpOtExtSender.h"
#include "Hashing/BitPosition.h"
//#define PRINT
namespace osuCrypto
{
    OPPRFReceiver::OPPRFReceiver()
    {
    }

    OPPRFReceiver::~OPPRFReceiver()
    {
    }

    void OPPRFReceiver::init(u64 numParties,
        u64 n,
        u64 statSec,
        u64 inputBitSize,
        Channel & chl0,
		NcoOtExtReceiver& ots,
		NcoOtExtSender& otSend,
        block seed)
    {
        init(numParties,n, statSec, inputBitSize, { &chl0 }, ots, otSend, seed);
    }

    void OPPRFReceiver::init(u64 numParties,
			u64 n,
			u64 statSecParam,
			u64 inputBitSize,
			const std::vector<Channel*>& chls,
			NcoOtExtReceiver& otRecv,
			NcoOtExtSender& otSend,
			block seed)
	{

		// this is the offline function for doing binning and then performing the OtPsi* between the bins.

		mParties = numParties;
		mStatSecParam = statSecParam;
		mN = n;

		// must be a multiple of 128...
		u64 baseOtCount;// = 128 * CodeWordSize;
		u64 compSecParam = 128;

		otSend.getParams(
			false,
			compSecParam, statSecParam, inputBitSize, mN, //  input
			mNcoInputBlkSize, baseOtCount); // output

											//mOtMsgBlkSize = (baseOtCount + 127) / 128;


		gTimer.setTimePoint("Init.recv.start");
		mPrng.SetSeed(seed);
		auto& prng = mPrng;

		auto myHashSeed = prng.get<block>();

		auto& chl0 = *chls[0];

		// we need a random hash function, so we will both commit to a seed and then later decommit. 
		//This is the commitments phase
		Commit comm(myHashSeed), theirComm;
		chl0.asyncSend(comm.data(), comm.size());
		chl0.recv(theirComm.data(), theirComm.size());

		// ok, now decommit to the seed.
		chl0.asyncSend(&myHashSeed, sizeof(block));
		block theirHashingSeed;
		chl0.recv(&theirHashingSeed, sizeof(block));

		gTimer.setTimePoint("Init.recv.hashSeed");

		// compute the hashing seed as the xor of both of ours seeds.
		mHashingSeed = myHashSeed ^ theirHashingSeed;


		// this SimpleHasher class knows how to hash things into bins. But first we need 
		// to compute how many bins we need, the max size of bins, etc.
		mSimpleBins.init(mParties,n, inputBitSize, mHashingSeed, statSecParam);
		mCuckooBins.init(mParties,n, mHashingSeed, statSecParam, false);

		// figure out how many OTs we need in total.
		//u64 otCount = mSimpleBins.mBinCount;//+mSimpleBins.mBinCount;
		u64 otCountSend = mSimpleBins.mBins.size();
		u64 otCountRecv = mCuckooBins.mBins.size();


		gTimer.setTimePoint("Init.recv.baseStart");
		// since we are doing mmlicious PSI, we need OTs going in both directions. 
		// This will hold the send OTs

		if (otRecv.hasBaseOts() == false ||
			otSend.hasBaseOts() == false)
		{
			// first do 128 public key OTs (expensive)
			std::array<block, gOtExtBaseOtCount> kosSendBase;
			BitVector choices(gOtExtBaseOtCount); choices.randomize(prng);
			NaorPinkas base;
			base.receive(choices, kosSendBase, prng, chl0, 2);


			IknpOtExtSender kosSend;
			kosSend.setBaseOts(kosSendBase, choices);
			std::vector<std::array<block, 2>> sendBaseMsg(baseOtCount + gOtExtBaseOtCount);
			kosSend.send(sendBaseMsg, prng, chl0);


			// Divide these OT mssages between the Kco and Kos protocols
			ArrayView<std::array<block, 2>> kcoRecvBase(
				sendBaseMsg.begin(),
				sendBaseMsg.begin() + baseOtCount);
			ArrayView<std::array<block, 2>> kosRecvBase(
				sendBaseMsg.begin() + baseOtCount,
				sendBaseMsg.end());

			BitVector recvChoice(baseOtCount); recvChoice.randomize(prng);
			std::vector<block> kcoSendBase(baseOtCount);
			IknpOtExtReceiver kos;
			kos.setBaseOts(kosRecvBase);
			kos.receive(recvChoice, kcoSendBase, prng, chl0);

			// now set these ~800 OTs as the base of our N choose 1 OTs.
			otSend.setBaseOts(kcoSendBase, recvChoice);

			// now set these ~800 OTs as the base of our N choose 1 OTs.
			otRecv.setBaseOts(kcoRecvBase);
		}


		gTimer.setTimePoint("Init.recv.ExtStart");




		auto sendOtRoutine = [&](u64 tIdx, u64 total, NcoOtExtSender& ots, Channel& chl)
		{
			auto start = (tIdx     *otCountSend / total) ;
			auto end = ((tIdx + 1) * otCountSend / total);

			ots.init(end - start);
		};

		auto recvOtRoutine = [&](u64 tIdx, u64 total, NcoOtExtReceiver& ots, Channel& chl)
		{
			auto start = (tIdx     * otCountRecv / total) ;
			auto end = ((tIdx + 1) * otCountRecv / total) ;

			ots.init(end - start);
		};


		// compute how amny threads we want to do for each direction.
		// the current thread will do one of the OT receives so -1 for that.
		u64 numThreads = chls.size() - 1;
		u64 numRecvThreads = numThreads / 2;
		u64 numSendThreads = numThreads - numRecvThreads;

		// where we will store the threads that are doing the extension
		std::vector<std::thread> thrds(numThreads);

		// some iters to help giving out resources.
		auto thrdIter = thrds.begin();
		auto chlIter = chls.begin() + 1;

		mOtRecvs.resize(chls.size());

		// now make the threads that will to the extension
		for (u64 i = 0; i < numRecvThreads; ++i)
		{
			mOtRecvs[i + 1] = std::move(otRecv.split());

			// spawn the thread and call the routine.
			*thrdIter++ = std::thread([&, i, chlIter]()
			{
				recvOtRoutine(i + 1, numRecvThreads + 1, *mOtRecvs[i + 1], **chlIter);
			});

			++chlIter;
		}

		mOtSends.resize(chls.size());
		// do the same thing but for the send OT extensions
		for (u64 i = 0; i < numSendThreads; ++i)
		{

			mOtSends[i] = std::move(otSend.split());

			*thrdIter++ = std::thread([&, i, chlIter]()
			{
				sendOtRoutine(i, numSendThreads, *mOtSends[i], **chlIter);
			});

			++chlIter;
		}

		mOtRecvs[0] = std::move(otRecv.split());

		// now use this thread to do a recv routine.
		recvOtRoutine(0, numRecvThreads + 1, *mOtRecvs[0], chl0);

		// if the caller doesnt want to do things in parallel
		// the we will need to do the send OT Ext now...
		if (numSendThreads == 0)
		{
			mOtSends[0] = std::move(otSend.split());

			sendOtRoutine(0, 1, *mOtSends[0], chl0);
		}

		// join any threads that we created.
		for (auto& thrd : thrds)
			thrd.join();

		gTimer.setTimePoint("Init.recv.done");

	}


	void OPPRFReceiver::hash2Bins(std::vector<block>& inputs, Channel & chl)
    {
		hash2Bins(inputs,{ &chl });
    }

	void OPPRFReceiver::hash2Bins(std::vector<block>& inputs, const std::vector<Channel*>& chls)
    {
#if 1
        // this is the online phase.
        gTimer.setTimePoint("online.recv.start");

        // check that the number of inputs is as expected.
        if (inputs.size() != mN)
            throw std::runtime_error(LOCATION);

		u64 maskSize = roundUpTo(mStatSecParam + 2 * std::log(mN) - 1, 8) / 8;


        if (maskSize > sizeof(block))
            throw std::runtime_error("masked are stored in blocks, so they can exceed that size");


        std::vector<std::thread>  thrds(chls.size());
      //  std::vector<std::thread>  thrds(1);

        // since we are going to do this in parallel, these objects will
        // be used for synchronization. specifically, when all threads are 
        // done inserting items into the bins, the future will be fulfilled 
        // and all threads will advance to performing the base OtPsi's
        std::atomic<u32>
            insertRemaining((u32)thrds.size());

		std::promise<void> insertProm ;
		std::shared_future<void>
			insertFuture(insertProm.get_future());

      //  CuckooHasher1 maskMap;
        //maskMap.init(mN * mBins.mMaxBinSize, mStatSecParam, chls.size() > 1);

		// this mutex is used to guard inserting things into the bin
	   std::mutex mInsertBin;
	   
	   mNcoInputBuff.resize(mNcoInputBlkSize);

        for (u64 hashIdx = 0; hashIdx < mNcoInputBuff.size(); ++hashIdx)
			mNcoInputBuff[hashIdx].resize(mN);


        // fr each thread, spawn it.
        for (u64 tIdx = 0; tIdx < thrds.size(); ++tIdx)
        {
            auto seed = mPrng.get<block>();
            thrds[tIdx] = std::thread([&, tIdx, seed]()
            {

                if (tIdx == 0) gTimer.setTimePoint("online.recv.thrdStart");

                auto& chl = *chls[tIdx];

                auto startIdx = tIdx     * mN / thrds.size();
                auto endIdx = (tIdx + 1) * mN / thrds.size();
#pragma region Hashing


                std::vector<AES> ncoInputHasher(mNcoInputBlkSize);
                for (u64 i = 0; i < ncoInputHasher.size(); ++i)
                    ncoInputHasher[i].setKey(_mm_set1_epi64x(i) ^ mHashingSeed);

				//Log::out << "mHashingSeed: " << mHashingSeed << Log::endl;

                u64 phaseShift = log2ceil(mN) / 8;

                for (u64 i = startIdx; i < endIdx; i += 128)
                {
                    auto currentStepSize = std::min(u64(128), mN - i);

                    for (u64 hashIdx = 0; hashIdx < ncoInputHasher.size(); ++hashIdx)
                    {
                        ncoInputHasher[hashIdx].ecbEncBlocks(
							inputs.data() + i,
                            currentStepSize,
							mNcoInputBuff[hashIdx].data() + i);
                    }
					
					
                    // since we are using random codes, lets just use the first part of the code 
                    // as where each item should be hashed.
				//	std::vector<block> tempMaskBuff(currentStepSize);

					std::vector<block> tempMaskBuff(currentStepSize);
					std::vector<u64> tempIdxBuff(currentStepSize); 
					CuckooHasher1::Workspace w(tempMaskBuff.size());
					MatrixView<u64> hashes(currentStepSize,mCuckooBins.mParams.mNumHashes);

                    for (u64 j = 0; j < currentStepSize; ++j)
                    {
						tempIdxBuff[j] = i + j;
						for (u64 k = 0; k <mCuckooBins.mParams.mNumHashes; ++k)
						{
							hashes[j][k] = *(u64*)&mNcoInputBuff[k][i + j];
						}		                 					
					}

					mSimpleBins.insertBatch(tempIdxBuff, hashes);
					mCuckooBins.insertBatch(tempIdxBuff, hashes, w);
				
                }

				CuckooHasher1::Workspace stashW(mCuckooBins.mStashIdxs.size());
				MatrixView<u64> stashHashes(mCuckooBins.mStashIdxs.size(), mCuckooBins.mParams.mNumHashes);

				for (u64 j = 0; j < mCuckooBins.mStashIdxs.size(); ++j)
				{
					for (u64 k = 0; k <mCuckooBins.mParams.mNumStashHashes; ++k)
					{
						stashHashes[j][k] = *(u64*)&mNcoInputBuff[k][mCuckooBins.mStashIdxs[j]];
					}
				}
				mCuckooBins.insertStashBatch(mCuckooBins.mStashIdxs, stashHashes, stashW);

                // block until all items have been inserted. the last to finish will set the promise...
                if (--insertRemaining)
                    insertFuture.get();
                else
                    insertProm.set_value();




				/*for (u64 i = 0; i < mBins.mBinCount; ++i)
				{
					if (i < 3 || (i < mN && i > mN - 2)) {
						
							std::cout << "r-Bin" << i << ": " << mBins.mBins[i].idx();
					
						std::cout << std::endl;
					}
				}*/

			//	mBins.print();

				if (tIdx == 0) gTimer.setTimePoint("online.recv.thrdStart");

#pragma region Init Bin
#if 1
				if (tIdx == 0) gTimer.setTimePoint("online.recv.insertDone");


				if (mCuckooBins.mBins.size() != mSimpleBins.mBins.size())
					throw std::runtime_error("mCuckooBins.mBins.size()!= mSimpleBins.mBins.size()");

				auto binCount = mCuckooBins.mBins.size();
				// get the region of the base OTs that this thread should do.
				auto binStart = tIdx       * binCount / thrds.size();
				auto binEnd = (tIdx + 1) * binCount / thrds.size();

				const u64 stepSize = 16;

				for (u64 bIdx = binStart; bIdx < binEnd;)
				{
					u64 currentStepSize = std::min(stepSize, binEnd - bIdx);

					for (u64 stepIdx = 0; stepIdx < currentStepSize; ++bIdx, ++stepIdx)
					{
						auto& cbin = mCuckooBins.mBins[bIdx];
						if (!cbin.isEmpty())
						{
							cbin.mValOPRF.resize(mParties);
							cbin.mValMap.resize(mParties);
						}
						auto& sbin = mSimpleBins.mBins[bIdx];

						if (sbin.mIdx.size()>0)
						{
							sbin.mValOPRF.resize(mParties);
							sbin.mBits.resize(mParties);
						}

					}

				}
#endif
#pragma endregion

#pragma endregion
            });      

		
		}



        // join the threads.
        for (u64 tIdx = 0; tIdx < thrds.size(); ++tIdx)
            thrds[tIdx].join();

        gTimer.setTimePoint("online.recv.exit");

		
        //std::cout << gTimer;
#endif
    }

	void OPPRFReceiver::getOPRFkeys(u64 IdxParty, Channel & chl)
	{
		getOPRFkeys( IdxParty,{ &chl });
	}

	void OPPRFReceiver::getOPRFkeys(u64 IdxP, const std::vector<Channel*>& chls)
	{
#if 1
		// this is the online phase.
		gTimer.setTimePoint("online.recv.start");

		u64 maskSize = roundUpTo(mStatSecParam + 2 * std::log(mN) - 1, 8) / 8;

		if (maskSize > sizeof(block))
			throw std::runtime_error("masked are stored in blocks, so they can exceed that size");

		std::vector<std::thread>  thrds(chls.size());
		//  std::vector<std::thread>  thrds(1);

		// fr each thread, spawn it.
		for (u64 tIdx = 0; tIdx < thrds.size(); ++tIdx)
		{
			auto seed = mPrng.get<block>();
			thrds[tIdx] = std::thread([&, tIdx, seed]()
			{

				if (tIdx == 0) gTimer.setTimePoint("online.recv.thrdStart");

				auto& otRecv = *mOtRecvs[tIdx];
				auto& otSend = *mOtSends[tIdx];
				auto& chl = *chls[tIdx];
				
				if (tIdx == 0) gTimer.setTimePoint("online.recv.insertDone");

				const u64 stepSize = 16;
				std::vector<block> ncoInput(mNcoInputBlkSize);

#pragma region compute Recv Bark-OPRF
#if 1
				//####################
				//#######Recv role
				//####################


				auto otCountRecv = mCuckooBins.mBins.size();
				// get the region of the base OTs that this thread should do.
				auto binStart = tIdx       * otCountRecv / thrds.size();
				auto binEnd = (tIdx + 1) * otCountRecv / thrds.size();

				//if (!tIdx)
				//gTimer.setTimePoint("sendInput.PSI");			

				

				for (u64 bIdx = binStart; bIdx < binEnd;)
				{
					u64 currentStepSize = std::min(stepSize, binEnd - bIdx);

					for (u64 stepIdx = 0; stepIdx < currentStepSize; ++bIdx, ++stepIdx)
					{

						auto& bin = mCuckooBins.mBins[bIdx];

						if (!bin.isEmpty())
						{
							u64 inputIdx = bin.idx();

							for (u64 j = 0; j < ncoInput.size(); ++j)
								ncoInput[j] = mNcoInputBuff[j][inputIdx];

							otRecv.encode(
								bIdx,      // input
								ncoInput,             // input
								bin.mValOPRF[IdxP]); // output

											   /*if (bIdx < 3 || (bIdx < mN && bIdx > mN-2))
											   std::cout << "r-" << bIdx << ", " << inputIdx << ": " << valOPRF[inputIdx] << std::endl;*/
						}
						else
						{
							otRecv.zeroEncode(bIdx);
						}
					}
					otRecv.sendCorrection(chl, currentStepSize);
				}

				  if (tIdx == 0) gTimer.setTimePoint("online.recv.otRecv.finalOPRF");


#endif
#pragma endregion

#if 1
#pragma region compute Send Bark-OPRF				
				//####################
				//#######Sender role
				//####################

				auto otCountSend = mSimpleBins.mBins.size();

				 binStart = tIdx       * otCountSend / thrds.size();
				 binEnd = (tIdx + 1) * otCountSend / thrds.size();


				if (tIdx == 0) gTimer.setTimePoint("online.send.OT");

				for (u64 bIdx = binStart; bIdx < binEnd;)
				{
					u64 currentStepSize = std::min(stepSize, binEnd - bIdx);
					otSend.recvCorrection(chl, currentStepSize);

					for (u64 stepIdx = 0; stepIdx < currentStepSize; ++bIdx, ++stepIdx)
					{

						auto& bin = mSimpleBins.mBins[bIdx];

						if (bin.mIdx.size() > 0)
						{
							bin.mValOPRF[IdxP].resize(bin.mIdx.size());

							std::cout << "s-" << bIdx << ", ";
							for (u64 i = 0; i < bin.mIdx.size(); ++i)
							{

								u64 inputIdx = bin.mIdx[i];

								for (u64 j = 0; j < mNcoInputBlkSize; ++j)
								{
									ncoInput[j] = mNcoInputBuff[j][inputIdx];
								}

								//    block sendMask;

								otSend.encode(
									bIdx, //each bin has 1 OT
									ncoInput,
									bin.mValOPRF[IdxP][i]);

								/*if (bIdx < 3 || (bIdx < mN && bIdx > mN - 2))
								std::cout << "s-"<<bIdx <<", "<< inputIdx << ": " << sendMask << std::endl;*/
							}

							//#####################
							//######Finding bit locations
							//#####################

							std::cout << bin.mValOPRF[IdxP][0];

							//diff max bin size for first mSimpleBins.mBinCount and 
							// mSimpleBins.mBinStashCount
							if (bIdx < mSimpleBins.mBinCount)
								bin.mBits[IdxP].init(bin.mIdx.size(), mSimpleBins.mNumBits);
							else
								bin.mBits[IdxP].init(bin.mIdx.size(), mSimpleBins.mNumStashBits);

							bin.mBits[IdxP].getPos(bin.mValOPRF[IdxP], 128);
							bin.mBits[IdxP].getMasks(bin.mValOPRF[IdxP]);
							std::cout << ", "
								<< static_cast<int16_t>(bin.mBits[IdxP].mMaps[0]) << std::endl;
						}
					}
				}

				if (tIdx == 0) gTimer.setTimePoint("online.send.otSend.finalOPRF");

#pragma endregion
#endif
				otRecv.check(chl);
				otSend.check(chl);
			});
		}

		// join the threads.
		for (u64 tIdx = 0; tIdx < thrds.size(); ++tIdx)
			thrds[tIdx].join();

		gTimer.setTimePoint("online.recv.exit");


		//std::cout << gTimer;
#endif
	}


#if 0
	void OPPRFReceiver::decrypt(std::vector<block>& plaintexts, Channel & chl)
	{
		return decrypt(plaintexts, { &chl });
	}

	void OPPRFReceiver::decrypt(std::vector<block>& plaintexts, const std::vector<Channel*>& chls)
	{

		// this is the online phase.
		gTimer.setTimePoint("online.recv.start");

		u64 maskSize = sizeof(block);// roundUpTo(mStatSecParam + 2 * std::log(mN) - 1, 8) / 8;


		if (maskSize > sizeof(block))
			throw std::runtime_error("masked are stored in blocks, so they can exceed that size");


		std::vector<std::thread>  thrds(chls.size());
		// this mutex is used to guard inserting things into the intersection vector.
		std::mutex mInsertMtx;

		// fr each thread, spawn it.
		for (u64 tIdx = 0; tIdx < thrds.size(); ++tIdx)
		{
			auto seed = mPrng.get<block>();
			thrds[tIdx] = std::thread([&, tIdx, seed]()
			{
				if (tIdx == 0) gTimer.setTimePoint("online.recv.thrdStart");

				auto& otRecv = *mOtRecvs[tIdx];

				auto& chl = *chls[tIdx];

				auto startIdx = tIdx     * mN / thrds.size();
				auto endIdx = (tIdx + 1) * mN / thrds.size();

				auto binStart = tIdx       * mBins.mBinCount / thrds.size();
				auto binEnd = (tIdx + 1) * mBins.mBinCount / thrds.size();
				const u64 stepSize = 16;
								

				if (tIdx == 0) gTimer.setTimePoint("online.recv.sendMask");

				// all masks have been merged
				// this is the intersection that will be computed by this thread,
				// this will be merged into the overall list at the end.
				u64 localIntersection = -1;
				u64 mTheirBins_mMaxBinSize = 32;
				u64 mTheirBins_mNumBits = 5;

				for (u64 bIdx = binStart; bIdx < binEnd;)
				{
					u64 curStepSize = std::min(stepSize, binEnd - bIdx);

					MatrixView<u8> maskView;
					ByteStream maskBuffer;
					chl.recv(maskBuffer);
					//maskView = maskBuffer.getMatrixView<u8>(mTheirBins_mMaxBinSize * maskSize + mTheirBins_mNumBits * sizeof(u8));
					maskView = maskBuffer.getMatrixView<u8>(mTheirBins_mMaxBinSize * maskSize + mTheirBins_mNumBits * sizeof(u8));
					if (maskView.size()[0] != curStepSize)
						throw std::runtime_error("size not expedted");

					for (u64 stepIdx = 0; stepIdx < curStepSize; ++bIdx, ++stepIdx)
					{

						auto& bin = mBins.mBins[bIdx];
						if (!bin.isEmpty())
						{
							u64 baseMaskIdx = stepIdx;
							auto mask = maskView[baseMaskIdx];
							BitPosition b;
							b.mMaxBitSize = mTheirBins_mNumBits;
							for (u64 i = 0; i < b.mMaxBitSize; i++)
							{
								int idxPos = 0;
								memcpy(&idxPos, maskView[baseMaskIdx].data()+i, sizeof(u8));
								b.mPos.push_back(idxPos);
							}
#ifdef PRINT
							Log::out << "RBin #" << bIdx << Log::endl;
							Log::out << "    cc_mPos= ";
							for (u64 idxPos = 0; idxPos < b.mPos.size(); idxPos++)
							{
								Log::out << static_cast<int16_t>(b.mPos[idxPos]) << " ";
							}
							Log::out << Log::endl;
#endif
							u64 inputIdx = bin.idx();
							auto myMask = bin.mValOPRF;
						//	u8 myMaskPos = 0;
							b.getMask(myMask, bin.mValMap);

							u64	MaskIdx = bin.mValMap*maskSize + mTheirBins_mNumBits;

								auto theirMask = ZeroBlock;
								memcpy(&theirMask, maskView[baseMaskIdx].data() + MaskIdx, maskSize);

								//if (!memcmp((u8*)&myMask, &theirMask, maskSize))
								//{
									//Log::out << "inputIdx: " << inputIdx << Log::endl;
								//	Log::out << "myMask: " << myMask << Log::endl;
									//Log::out << "theirMask: " << theirMask << " " << Log::endl;
									plaintexts[inputIdx] = myMask^theirMask;

									localIntersection = inputIdx;

								//}
						}
					}
			
					if (localIntersection != -1)
					{
						std::lock_guard<std::mutex> lock(mInsertMtx);
						if (mIntersection.size())
						{
							mIntersection.push_back(localIntersection);
						}
					}
				}				

			});
		//	if (tIdx == 0) gTimer.setTimePoint("online.recv.done");
		}
		// join the threads.
		for (auto& thrd : thrds)
			thrd.join();
		
		// check that the number of inputs is as expected.
		if (plaintexts.size() != mN)
			throw std::runtime_error(LOCATION);

		

	}

#endif

}
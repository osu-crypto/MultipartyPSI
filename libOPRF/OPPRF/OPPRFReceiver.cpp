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

    void OPPRFReceiver::init(
        u64 n,
        u64 statSec,
        u64 inputBitSize,
        Channel & chl0,
		NcoOtExtReceiver& ots,
		NcoOtExtSender& otSend,
        block seed)
    {
        init(n, statSec, inputBitSize, { &chl0 }, ots, otSend, seed);
    }

    void OPPRFReceiver::init(
			u64 n,
			u64 statSecParam,
			u64 inputBitSize,
			const std::vector<Channel*>& chls,
			NcoOtExtReceiver& otRecv,
			NcoOtExtSender& otSend,
			block seed)
	{

		// this is the offline function for doing binning and then performing the OtPsi* between the bins.


		mStatSecParam = statSecParam;
		mN = n;

		// must be a multiple of 128...
		u64 baseOtCount;// = 128 * CodeWordSize;
		u64 compSecParam = 128;

		otSend.getParams(
			true,
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
		mSimpleBins.init(n, inputBitSize, mHashingSeed, statSecParam, true);

		// figure out how many OTs we need in total.
		u64 otCount = mSimpleBins.mBinCount;


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
			auto start = (tIdx     *otCount / total) ;
			auto end = ((tIdx + 1) * otCount / total);

			ots.init(end - start);
		};

		auto recvOtRoutine = [&](u64 tIdx, u64 total, NcoOtExtReceiver& ots, Channel& chl)
		{
			auto start = (tIdx     * otCount / total) ;
			auto end = ((tIdx + 1) * otCount / total) ;

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

#if 0
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


        
        std::vector<block> valOPRF(mN);
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

		std::promise<void> insertProm , maskMergeProm;
		std::shared_future<void>
			insertFuture(insertProm.get_future()),
           maskMergeFuture(maskMergeProm.get_future());

        std::promise<MatrixView<u8>> maskProm;
       std::shared_future<MatrixView<u8>> maskFuture(maskProm.get_future());
       // ByteStream maskBuffer;


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

                auto& otRecv = *mOtRecvs[tIdx];


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
					MatrixView<u64> hashes(currentStepSize,mBins.mParams.mNumHashes);

                    for (u64 j = 0; j < currentStepSize; ++j)
                    {
						tempIdxBuff[j] = i + j;
						for (u64 k = 0; k <mBins.mParams.mNumHashes; ++k)
						{
							hashes[j][k] = *(u64*)&mNcoInputBuff[k][i + j];
						}		                 					
					}


				//	std::lock_guard<std::mutex> lock(mInsertBin);
					mBins.insertBatch(tempIdxBuff, hashes, w,true);	
                }
				

				CuckooHasher1::Workspace stashW(mBins.mStashIdxs.size());
				MatrixView<u64> stashHashes(mBins.mStashIdxs.size(), mBins.mParams.mNumHashes);

				for (u64 j = 0; j < mBins.mStashIdxs.size(); ++j)
				{
					for (u64 k = 0; k <mBins.mParams.mNumHashes; ++k)
					{
						stashHashes[j][k] = *(u64*)&mNcoInputBuff[k][mBins.mStashIdxs[j]];
					}
				}
				mStashBins.insertBatch(mBins.mStashIdxs, stashHashes, stashW, false);

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

	void OPPRFReceiver::getOPRFkeys(std::vector<block>& inputs, Channel & chl)
	{
		getOPRFkeys(inputs, { &chl });
	}

	void OPPRFReceiver::getOPRFkeys(std::vector<block>& inputs, const std::vector<Channel*>& chls)
	{
#if 1
		// this is the online phase.
		gTimer.setTimePoint("online.recv.start");

		// check that the number of inputs is as expected.
		if (inputs.size() != mN)
			throw std::runtime_error(LOCATION);



		std::vector<block> valOPRF(mN);
		u64 maskSize = roundUpTo(mStatSecParam + 2 * std::log(mN) - 1, 8) / 8;


		if (maskSize > sizeof(block))
			throw std::runtime_error("masked are stored in blocks, so they can exceed that size");


		std::vector<std::thread>  thrds(chls.size());
		//  std::vector<std::thread>  thrds(1);

		// since we are going to do this in parallel, these objects will
		// be used for synchronization. specifically, when all threads are 
		// done inserting items into the bins, the future will be fulfilled 
		// and all threads will advance to performing the base OtPsi's

		std::promise<void>  maskMergeProm;
		std::shared_future<void>
			maskMergeFuture(maskMergeProm.get_future());

		std::promise<MatrixView<u8>> maskProm;
		std::shared_future<MatrixView<u8>> maskFuture(maskProm.get_future());
		// ByteStream maskBuffer;


		//  CuckooHasher1 maskMap;
		//maskMap.init(mN * mBins.mMaxBinSize, mStatSecParam, chls.size() > 1);


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

#pragma region compute Bark-OPRF
#if 1
				if (tIdx == 0) gTimer.setTimePoint("online.recv.insertDone");

				// get the region of the base OTs that this thread should do.
				auto binStart = tIdx       * mBins.mBinCount / thrds.size();
				auto binEnd = (tIdx + 1) * mBins.mBinCount / thrds.size();
				auto otStart = binStart * 1;
				auto otEnd = binEnd * 1;

				PRNG prng(seed);

				//if (!tIdx)
				//gTimer.setTimePoint("sendInput.PSI");

				std::vector<u16> perm(mBins.mMaxBinSize);
				for (size_t i = 0; i < perm.size(); i++)
					perm[i] = i;


				const u64 stepSize = 16;


				u64 otIdx = 0;

				std::vector<block> ncoInput(mNcoInputBlkSize);

				for (u64 bIdx = binStart; bIdx < binEnd;)
				{
					u64 currentStepSize = std::min(stepSize, binEnd - bIdx);

					for (u64 stepIdx = 0; stepIdx < currentStepSize; ++bIdx, ++stepIdx)
					{

						auto& bin = mBins.mBins[bIdx];

						if (!bin.isEmpty())
						{
							u64 inputIdx = bin.idx();

							for (u64 j = 0; j < ncoInput.size(); ++j)
								ncoInput[j] = mNcoInputBuff[j][inputIdx];

							otRecv.encode(
								bIdx,      // input
								ncoInput,             // input
								bin.mValOPRF); // output

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


				//  if (tIdx == 0) gTimer.setTimePoint("online.recv.recvMask");
				otRecv.check(chl);

#endif
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
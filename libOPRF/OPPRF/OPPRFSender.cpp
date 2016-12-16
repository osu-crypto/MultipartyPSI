#include "OPPRFSender.h"

#include "Crypto/Commit.h"
#include "Common/Log.h"
#include "Common/Log1.h"
#include "Common/Timer.h"
#include "Base/naor-pinkas.h"
#include "TwoChooseOne/IknpOtExtReceiver.h"
#include "TwoChooseOne/IknpOtExtSender.h"

//#define PRINT
namespace osuCrypto
{

    OPPRFSender::OPPRFSender()
    {
    }
    //const u64 OPPRFSender::hasherStepSize(128);


    OPPRFSender::~OPPRFSender()
    {
    }

    void OPPRFSender::init(u64 numParties, u64 setSize, u64 statSec, u64 inputBitSize,
        Channel & chl0,
        NcoOtExtSender&  ots,
		NcoOtExtReceiver& otRecv,
        block seed)
    {
        init(numParties, setSize,  statSec, inputBitSize, { &chl0 }, ots, otRecv, seed);
    }

    void OPPRFSender::init(u64 numParties, u64 setSize,  u64 statSec, u64 inputBitSize,
        const std::vector<Channel*>& chls,
        NcoOtExtSender& otSend,
		NcoOtExtReceiver& otRecv,
        block seed)
	{
		mStatSecParam = statSec;
		mN = setSize;
		mParties = numParties;
		gTimer.setTimePoint("init.send.start");

		// must be a multiple of 128...
		u64 baseOtCount;// = 128 * CodeWordSize;
						//u64 plaintextBlkSize;

		u64 compSecParam = 128;

		otSend.getParams(
			false, // input, is malicious
			compSecParam, statSec, inputBitSize, mN, //  input
			mNcoInputBlkSize, baseOtCount); // output

		mOtMsgBlkSize = (baseOtCount + 127) / 128;


		mPrng.SetSeed(seed);
		auto myHashSeed = mPrng.get<block>();
		auto& chl0 = *chls[0];


		Commit comm(myHashSeed), theirComm;
		chl0.asyncSend(comm.data(), comm.size());
		chl0.recv(theirComm.data(), theirComm.size());


		chl0.asyncSend(&myHashSeed, sizeof(block));
		block theirHashingSeed;
		chl0.recv(&theirHashingSeed, sizeof(block));

		mHashingSeed = myHashSeed ^ theirHashingSeed;

		gTimer.setTimePoint("init.send.hashSeed");


		mSimpleBins.init(mParties,mN, inputBitSize, mHashingSeed, statSec);
		mCuckooBins.init(mParties, mN, mHashingSeed, statSec, false);

		//mPsis.resize(mBins.mBinCount);

		
		u64 otCountSend = mSimpleBins.mBins.size();
		u64 otCountRecv=  mCuckooBins.mBins.size();


		gTimer.setTimePoint("init.send.baseStart");

		if (otSend.hasBaseOts() == false ||
			otRecv.hasBaseOts() == false)
		{
			// first do 128 public key OTs (expensive)
			std::array<std::array<block, 2>, gOtExtBaseOtCount> baseMsg;
			NaorPinkas base;
			base.send(baseMsg, mPrng, chl0, 2);


			// now extend these to enough recv OTs to seed the send Kco and the send Kos ot extension
			BitVector recvChoice(baseOtCount + gOtExtBaseOtCount); recvChoice.randomize(mPrng);
			std::vector<block> recvBaseMsg(baseOtCount + gOtExtBaseOtCount);
			IknpOtExtReceiver kosRecv;
			kosRecv.setBaseOts(baseMsg);
			kosRecv.receive(recvChoice, recvBaseMsg, mPrng, chl0);


			// we now have a bunch of recv OTs, lets seed the NcoOtExtSender
			BitVector kcoSendBaseChoice;
			kcoSendBaseChoice.copy(recvChoice, 0, baseOtCount);
			ArrayView<block> kcoSendBase(
				recvBaseMsg.begin(),
				recvBaseMsg.begin() + baseOtCount);

			otSend.setBaseOts(kcoSendBase, kcoSendBaseChoice);


			// now lets extend these recv OTs in the other direction
			BitVector kosSendBaseChoice;
			kosSendBaseChoice.copy(recvChoice, baseOtCount, gOtExtBaseOtCount);
			ArrayView<block> kosSendBase(
				recvBaseMsg.begin() + baseOtCount,
				recvBaseMsg.end());
			IknpOtExtSender kos;
			kos.setBaseOts(kosSendBase, kosSendBaseChoice);

			// these send OTs will be stored here
			std::vector<std::array<block, 2>> sendBaseMsg(baseOtCount);
			kos.send(sendBaseMsg, mPrng, chl0);

			// now set these ~800 OTs as the base of our N choose 1 OTs NcoOtExtReceiver
			otRecv.setBaseOts(sendBaseMsg);
		}

		gTimer.setTimePoint("init.send.extStart");

		mOtSends.resize(chls.size());
		mOtRecvs.resize(chls.size());

		auto sendRoutine = [&](u64 tIdx, u64 total, NcoOtExtSender& ots, Channel& chl)
		{
			auto start = (tIdx     * otCountSend / total) ;
			auto end = ((tIdx + 1) * otCountSend / total) ;

			ots.init(end - start);
		};

		auto recvOtRountine = [&](u64 tIdx, u64 total, NcoOtExtReceiver& ots, Channel& chl)
		{
			auto start = (tIdx     * otCountRecv / total) ;
			auto end = ((tIdx + 1) * otCountRecv / total) ;

			ots.init(end - start);
		};

		u64 numThreads = chls.size() - 1;
		u64 numSendThreads = numThreads / 2;
		u64 numRecvThreads = numThreads - numSendThreads;


		std::vector<std::thread> thrds(numThreads);
		auto thrdIter = thrds.begin();
		auto chlIter = chls.begin() + 1;


		for (u64 i = 0; i < numSendThreads; ++i)
		{
			mOtSends[i] = std::move(otSend.split());

			*thrdIter++ = std::thread([&, i, chlIter]()
			{
				//std::cout << IoStream::lock << "s sendOt " << l << "  " << (**chlIter).getName() << std::endl << IoStream::unlock;
				sendRoutine(i + 1, numSendThreads + 1, *mOtSends[i], **chlIter);
			});
			++chlIter;
		}

		for (u64 i = 0; i < numRecvThreads; ++i)
		{
			mOtRecvs[i] = std::move(otRecv.split());

			*thrdIter++ = std::thread([&, i, chlIter]()
			{
				//std::cout << IoStream::lock << "s recvOt " << l << "  " << (**chlIter).getName() << std::endl << IoStream::unlock;
				recvOtRountine(i, numRecvThreads, *mOtRecvs[i], **chlIter);
			});
			++chlIter;
		}

		mOtSends[0] = std::move(otSend.split());
		sendRoutine(0, numSendThreads + 1, *mOtSends[0], chl0);

		if (numRecvThreads == 0)
		{
			mOtRecvs[0] = std::move(otRecv.split());

			recvOtRountine(0, 1, *mOtRecvs[0], chl0);
		}


		for (auto& thrd : thrds)
			thrd.join();

		gTimer.setTimePoint("init.send.done");

	}


	void OPPRFSender::hash2Bins(std::vector<block>& inputs, Channel & chl)
	{
		hash2Bins(inputs, { &chl });
	}
	void  OPPRFSender::hash2Bins(std::vector<block>& inputs, const std::vector<Channel*>& chls)
    {

        if (inputs.size() != mN)
            throw std::runtime_error(LOCATION);

		//TODO: double check
        u64 maskSize = roundUpTo(mStatSecParam + 2 * std::log(mN) - 1, 8) / 8;
		
		if (maskSize > sizeof(block))
            throw std::runtime_error("masked are stored in blocks, so they can exceed that size");



        std::vector<std::thread>  thrds(chls.size());
        //std::vector<std::thread>  thrds(1);        

		std::atomic<u32> remaining((u32)thrds.size());
		std::promise<void> doneProm ;
		std::shared_future<void>
			doneFuture(doneProm.get_future());

        std::mutex mtx;

		mNcoInputBuff.resize(mNcoInputBlkSize);

        for (u64 hashIdx = 0; hashIdx < mNcoInputBuff.size(); ++hashIdx)
			mNcoInputBuff[hashIdx].resize(mN);


        auto permSeed = mPrng.get<block>();

        
        gTimer.setTimePoint("online.send.spaw");

        for (u64 tIdx = 0; tIdx < thrds.size(); ++tIdx)
        {
            auto seed = mPrng.get<block>();
            thrds[tIdx] = std::thread([&, tIdx, seed]() {

                PRNG prng(seed);

                if (tIdx == 0) gTimer.setTimePoint("online.send.thrdStart");

                auto& otSend = *mOtSends[tIdx];
                auto startIdx = tIdx       * mN / thrds.size();
                auto endIdx = (tIdx + 1) * mN / thrds.size();

                // compute the region of inputs this thread should insert.
                //ArrayView<block> itemRange(
                //    inputs.begin() + startIdx,
                //    inputs.begin() + endIdx);
#pragma region Hashing				
                std::vector<AES> ncoInputHasher(mNcoInputBlkSize);
				//Log::out << "mHashingSeed: " << mHashingSeed << Log::endl;
                for (u64 i = 0; i < ncoInputHasher.size(); ++i)
                    ncoInputHasher[i].setKey(_mm_set1_epi64x(i) ^ mHashingSeed);

                

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
					std::vector<block> tempMaskBuff(currentStepSize);
					std::vector<u64> tempIdxBuff(currentStepSize);
					CuckooHasher1::Workspace w(tempMaskBuff.size());
					MatrixView<u64> hashes(currentStepSize, mCuckooBins.mParams.mNumHashes);

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
				MatrixView<u64> stashHashes(mCuckooBins.mStashIdxs.size(), mCuckooBins.mParams.mNumStashHashes);

				for (u64 j = 0; j < mCuckooBins.mStashIdxs.size(); ++j)
				{
					for (u64 k = 0; k <mCuckooBins.mParams.mNumStashHashes; ++k)
					{
						stashHashes[j][k] = *(u64*)&mNcoInputBuff[k][mCuckooBins.mStashIdxs[j]];
					}
				}
				mCuckooBins.insertStashBatch(mCuckooBins.mStashIdxs, stashHashes, stashW);

                //<< IoStream::lock << "Sender"<< std::endl;
                //mBins.insertItemsWithPhasing(range, mStatSecParam, inputs.size());


                // block until all items have been inserted. the last to finish will set the promise...
                if (--remaining)
                    doneFuture.get();
                else
                    doneProm.set_value();

				/*for (u64 i = 0; i < mBins.mBinCount; ++i)
				{
					if (i < 3 || (i < mN && i > mN - 2)) {
						std::cout << "s-Bin" << i << ": ";
						for (u64 k = 0; k < mBins.mBins[i].size(); ++k)
						{
							std::cout << mBins.mBins[i][k] << " ";
						}
						std::cout << std::endl;
					}
				}*/

#pragma endregion
				if(tIdx == 0) gTimer.setTimePoint("online.recv.thrdStart");

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


            });       
			
		
		}

        for (auto& thrd : thrds)
            thrd.join();
    }


	void OPPRFSender::getOPRFKeys(u64 IdxParty, Channel & chl)
	{
		getOPRFKeys(IdxParty,{ &chl });
	}

	void  OPPRFSender::getOPRFKeys(u64 IdxP, const std::vector<Channel*>& chls)
	{

		//TODO: double check
		u64 maskSize = roundUpTo(mStatSecParam + 2 * std::log(mN) - 1, 8) / 8;

		if (maskSize > sizeof(block))
			throw std::runtime_error("masked are stored in blocks, so they can exceed that size");

		//std::vector<std::thread>  thrds(chls.size());
		std::vector<std::thread>  thrds(1);

		gTimer.setTimePoint("online.send.spaw");

		for (u64 tIdx = 0; tIdx < thrds.size(); ++tIdx)
		{
			auto seed = mPrng.get<block>();
			thrds[tIdx] = std::thread([&, tIdx, seed]() {

				PRNG prng(seed);

				if (tIdx == 0) gTimer.setTimePoint("online.send.thrdStart");

				auto& otRecv = *mOtRecvs[tIdx];
				auto& otSend = *mOtSends[tIdx];

				auto& chl = *chls[tIdx];

				if (tIdx == 0) gTimer.setTimePoint("online.send.insert");
				const u64 stepSize = 16;
				std::vector<block> ncoInput(mNcoInputBlkSize);

#if 1
#pragma region compute Send Bark-OPRF				
				//####################
				//#######Sender role
				//####################

				auto otCountSend =  mSimpleBins.mBins.size();

				auto binStart = tIdx       * otCountSend / thrds.size();
				auto binEnd = (tIdx + 1) * otCountSend / thrds.size();

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

#pragma region compute Recv Bark-OPRF
#if 1
				//####################
				//#######Receiver role
				//####################

				auto otCountRecv = mCuckooBins.mBins.size();
				// get the region of the base OTs that this thread should do.
				 binStart = tIdx       * otCountRecv / thrds.size();
				binEnd = (tIdx + 1) * otCountRecv / thrds.size();

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

				if (tIdx == 0) gTimer.setTimePoint("online.send.otRecv.finalOPRF");
#endif
#pragma endregion

				otSend.check(chl);
				otRecv.check(chl);

			});
		}

		for (auto& thrd : thrds)
			thrd.join();
	}
#if 0
	void OPPRFSender::sendEnc(std::vector<block>& plaintexts, Channel & chl)
	{
		sendEnc(plaintexts,{ &chl });
	}

	void OPPRFSender::sendEnc(std::vector<block>& plaintexts,const std::vector<Channel*>& chls)
	{
		if (plaintexts.size() != mN)
			throw std::runtime_error(LOCATION);


		//TODO: double check
		u64 maskSize = sizeof(block);//roundUpTo(mStatSecParam + 2 * std::log(mN) - 1, 8) / 8;
		//u64 maskSize = 7;
		if (maskSize > sizeof(block))
			throw std::runtime_error("masked are stored in blocks, so they can exceed that size");



		std::vector<std::thread>  thrds(chls.size());
		// std::vector<std::thread>  thrds(1);        

		std::atomic<u32> remainingMasks((u32)thrds.size());
		std::promise<void> maskProm;
		std::shared_future<void>
			maskFuture(maskProm.get_future());

		std::mutex mtx;


		gTimer.setTimePoint("online.send.spaw");

		for (u64 tIdx = 0; tIdx < thrds.size(); ++tIdx)
		{
			auto seed = mPrng.get<block>();
			thrds[tIdx] = std::thread([&, tIdx, seed]() {

				PRNG prng(seed);

				if (tIdx == 0) gTimer.setTimePoint("online.send.thrdStart");

				auto& otSend = *mOtSends[tIdx];

				auto& chl = *chls[tIdx];
				auto startIdx = tIdx       * mN / thrds.size();
				auto endIdx = (tIdx + 1) * mN / thrds.size();

				// compute the region of inputs this thread should insert.
				//ArrayView<block> itemRange(
				//    inputs.begin() + startIdx,
				//    inputs.begin() + endIdx);


#pragma region compute encryption
#if 1
				if (tIdx == 0) gTimer.setTimePoint("online.send.insert");

				const u64 stepSize = 16;

				auto binStart = tIdx       * mBins.mBinCount / thrds.size();
				auto binEnd = (tIdx + 1) * mBins.mBinCount / thrds.size();
				
				if (tIdx == 0) gTimer.setTimePoint("online.send.OT");

				for (u64 bIdx = binStart; bIdx < binEnd;)
				{
					u64 currentStepSize = std::min(stepSize, binEnd - bIdx);
					uPtr<Buff> sendMaskBuff(new Buff);
					sendMaskBuff->resize(currentStepSize * (mBins.mMaxBinSize * maskSize+ mBins.mNumBits * sizeof(u8)));
					auto maskView = sendMaskBuff->getMatrixView<u8>(mBins.mMaxBinSize * maskSize + mBins.mNumBits * sizeof(u8));

					for (u64 stepIdx = 0; stepIdx < currentStepSize; ++bIdx, ++stepIdx)
					{

						auto& bin = mBins.mBins[bIdx];
						u64 baseMaskIdx = stepIdx;
						
						std::vector<u8> dummyPos;

						//copy bit locations in which all OPRF values are distinct
						int MaskIdx = 0;
						/*Log::out << "BBin #" << bIdx << Log::endl;
						Log::out << "    c_mPos= " ;*/

						if (bin.mBits.mPos.size() != mBins.mNumBits)
						{
							throw std::runtime_error("bin.mBits.mPos.size()!= mBins.mNumBits");
							Log::out << "bin.mBits.mPos.size()!= mBins.mNumBits" << Log::endl;
						}

						for (u64 idxPos = 0; idxPos < bin.mBits.mPos.size(); idxPos++)
						{
						//	Log::out << static_cast<int16_t>(bin.mBits.mPos[idxPos]) << " ";
								memcpy(
									maskView[baseMaskIdx].data()+ idxPos,
									(u8*)&bin.mBits.mPos[idxPos], sizeof(u8));
						}
						//Log::out << Log::endl;

					
						for (u64 i = 0; i < bin.mIdx.size(); ++i)
						{
							u64 inputIdx = bin.mIdx[i];
							block encr = bin.mValOPRF[i] ^ plaintexts[inputIdx];

							//Log::out << "    c_idx=" << inputIdx;
							//Log::out << "    c_OPRF=" << encr;
							//Log::out << "    c_Map=" << static_cast<int16_t>(bin.mBits.mMaps[i]);

							MaskIdx = bin.mBits.mMaps[i]* maskSize+ mBins.mNumBits;

							memcpy(
								maskView[baseMaskIdx].data()+ MaskIdx,
								(u8*)&encr,
								maskSize);

						//	Log::out << Log::endl;
						}

						//#####################
						//######Filling dummy mask
						//#####################
							
						for (u64 i = 0; i < mBins.mMaxBinSize ; ++i)
						{
							if (std::find(bin.mBits.mMaps.begin(), bin.mBits.mMaps.end(), i) == bin.mBits.mMaps.end())
							{
								MaskIdx = i* maskSize+ mBins.mNumBits;
							//	Log::out << "    cc_Map=" << i << Log::endl;
								memcpy(
									maskView[baseMaskIdx].data() + MaskIdx,
									(u8*)&ZeroBlock,  //make randome
									maskSize);
							}
						}

					}
					
#ifdef PRINT
					Log::out << "maskSize: ";
					for (size_t i = 0; i < maskView.size()[0]; i++)
					{
						for (size_t j = 0; j < mBins.mNumBits; j++)
						{
							Log::out << static_cast<int16_t>(maskView[i][j]) << " ";
						}
						Log::out << Log::endl;

						for (size_t j = 0; j < mBins.mMaxBinSize; j++) {
							auto theirMask = ZeroBlock;
							memcpy(&theirMask, maskView[i].data()+j*maskSize+ mBins.mNumBits, maskSize);
							if (theirMask != ZeroBlock)
							{
								Log::out << theirMask << " " << Log::endl;
							}
						}
					}
#endif
						chl.asyncSend(std::move(sendMaskBuff));

				}
				if (tIdx == 0) gTimer.setTimePoint("online.send.sendMask");

			//	otSend.check(chl);

				// block until all masks are computed. the last to finish will set the promise...
				if (--remainingMasks)
				{
					maskFuture.get();
				}
				else
				{
					maskProm.set_value();
				}

				/* if (tIdx == 0)
				chl.asyncSend(std::move(sendMaskBuff));*/

				if (tIdx == 0) gTimer.setTimePoint("online.send.finalMask");
#endif
#pragma endregion

			});
		}

		for (auto& thrd : thrds)
			thrd.join();

		//    permThrd.join();



	}
#endif
}



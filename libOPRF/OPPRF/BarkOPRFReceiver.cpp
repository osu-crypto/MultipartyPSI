#include "BarkOPRFReceiver.h"
#include <future>

#include "Crypto/PRNG.h"
#include "Crypto/Commit.h"

#include "Common/Log.h"
#include "Common/Log1.h"
#include "Base/naor-pinkas.h"
#include <unordered_map>

#include "TwoChooseOne/IknpOtExtReceiver.h"
#include "TwoChooseOne/IknpOtExtSender.h"


namespace osuCrypto
{
    BarkOPRFReceiver::BarkOPRFReceiver()
    {
    }

    BarkOPRFReceiver::~BarkOPRFReceiver()
    {
    }
#if 0
    void BarkOPRFReceiver::init(
        u64 n,
        u64 statSecParam,
        u64 inputBitSize,
        Channel & chl0,
        NcoOtExtReceiver& ots,
        block seed)
    {
        init(n, statSecParam, inputBitSize, { &chl0 }, ots, seed);
    }

    void BarkOPRFReceiver::init(
        u64 n,
        u64 statSecParam,
        u64 inputBitSize,
        const std::vector<Channel*>& chls,
        NcoOtExtReceiver& otRecv,
        block seed)
    {

        // this is the offline function for doing binning and then performing the OtPsi* between the bins.


        mStatSecParam = statSecParam;
        mN = n;

        // must be a multiple of 128...
        u64 baseOtCount;// = 128 * CodeWordSize;
        u64 compSecParam = 128;

		otRecv.getParams(
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


        // this SimpleHasher1 class knows how to hash things into bins. But first we need 
        // to compute how many bins we need, the max size of bins, etc.
        mBins.init(2,n, mHashingSeed, statSecParam, false);
		//mTheirBins.init(n, inputBitSize, mHashingSeed, statSecParam);

        // figure out how many OTs we need in total.
        u64 perBinOtCount = 1;
		u64 otCount = perBinOtCount * (mBins.mBinCount); //NOTE: assume we dont have stash size +mBins.mParams.mStashSize);


        gTimer.setTimePoint("Init.recv.baseStart");
        // since we are doing mmlicious PSI, we need OTs going in both directions. 
        // This will hold the send OTs

        if (otRecv.hasBaseOts() == false)
        {
            // first do 128 public key OTs (expensive)
            std::array<block, gOtExtBaseOtCount> IknpSendBase;
            BitVector choices(gOtExtBaseOtCount); choices.randomize(prng);
            NaorPinkas base;
            base.receive(choices, IknpSendBase, prng, chl0, 2);


            IknpOtExtSender IknpSend;
            IknpSend.setBaseOts(IknpSendBase, choices);
            std::vector<std::array<block, 2>> sendBaseMsg(baseOtCount);
            IknpSend.send(sendBaseMsg, prng, chl0);


            // Divide these OT mssages between the Kco and Kos protocols
            ArrayView<std::array<block, 2>> kcoRecvBase(
                sendBaseMsg.begin(),
                sendBaseMsg.begin() + baseOtCount);

            // now set these ~800 OTs as the base of our N choose 1 OTs.
            otRecv.setBaseOts(kcoRecvBase);
        }
        

        gTimer.setTimePoint("Init.recv.ExtStart");



        auto recvOtRoutine = [&](u64 tIdx, u64 total, NcoOtExtReceiver& ots, Channel& chl)
        {
            auto start = (tIdx     * mBins.mBinCount / total) ;
            auto end = ((tIdx + 1) * mBins.mBinCount / total) ;

            ots.init(end - start);
        };


        // compute how amny threads we want to do for each direction.
        // the current thread will do one of the OT receives so -1 for that.
        u64 numThreads = chls.size()-1;
        
        // where we will store the threads that are doing the extension
        std::vector<std::thread> thrds(numThreads);

        // some iters to help giving out resources.
        auto thrdIter = thrds.begin();
        auto chlIter = chls.begin() + 1;

        mOtRecvs.resize(chls.size());

        // now make the threads that will to the extension
        for (u64 i = 0; i < numThreads; ++i)
        {
            mOtRecvs[i+1] = std::move(otRecv.split());

            // spawn the thread and call the routine.
            *thrdIter++ = std::thread([&, i, chlIter]()
            {
                recvOtRoutine(i+1, numThreads+1,*mOtRecvs[i+1],**chlIter);
            });

            ++chlIter;
        }

   
        mOtRecvs[0] = std::move(otRecv.split());

        // now use this thread to do a recv routine.
        recvOtRoutine(0, numThreads + 1, *mOtRecvs[0],  chl0);

        // join any threads that we created.
        for (auto& thrd : thrds)
            thrd.join();

        gTimer.setTimePoint("Init.recv.done");

    }


    void BarkOPRFReceiver::sendInput(std::vector<block>& inputs, Channel & chl)
    {
        sendInput(inputs, { &chl });
    }

    void BarkOPRFReceiver::sendInput(std::vector<block>& inputs, const std::vector<Channel*>& chls)
    {
#if 1
        // this is the online phase.
        gTimer.setTimePoint("online.recv.start");

        // check that the number of inputs is as expected.
        if (inputs.size() != mN)
            throw std::runtime_error(LOCATION);


        
        std::vector<block> recvMasks(mN);
		u64 maskSize = 7;// = roundUpTo(mStatSecParam + 2 * std::log(mN) - 1, 8) / 8;


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
	   
        // this mutex is used to guard inserting things into the intersection vector.
        std::mutex mInsertMtx;

        std::vector<std::vector<block>> ncoInputBuff(mNcoInputBlkSize);

        for (u64 hashIdx = 0; hashIdx < ncoInputBuff.size(); ++hashIdx)
            ncoInputBuff[hashIdx].resize(inputs.size());


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
                    auto currentStepSize = std::min(u64(128), inputs.size() - i);

                    for (u64 hashIdx = 0; hashIdx < ncoInputHasher.size(); ++hashIdx)
                    {
                        ncoInputHasher[hashIdx].ecbEncBlocks(
                            inputs.data() + i,
                            currentStepSize,
                            ncoInputBuff[hashIdx].data() + i);
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
							hashes[j][k] = *(u64*)&ncoInputBuff[k][i + j];
						}		
					//	std::lock_guard<std::mutex> lock(mBins.mMtx[addr]);
					//	mBins.insert(i + j, hashes);                    					
					}


					std::lock_guard<std::mutex> lock(mInsertBin);
					mBins.insertBatch(tempIdxBuff, hashes, w);

					/*std::cout << tempMaskBuff[5]<<"\n";
					std::cout << tempMaskBuff[5];
*/
				//	std::vector<u64> idxs(i, i+ currentStepSize);
				//	CuckooHasher1::Workspace w(currentStepSize);

                }

                // block until all items have been inserted. the last to finish will set the promise...
                if (--insertRemaining)
                    insertFuture.get();
                else
                    insertProm.set_value();

				for (u64 i = 0; i < mBins.mBinCount[0]; ++i)
				{
					if (i < 3 || (i < mN && i > mN - 2)) {
						
							std::cout << "r-Bin" << i << ": " << mBins.mBins[i].idx();
					
						std::cout << std::endl;
					}
				}

			//	mBins.print();

#pragma endregion

#pragma region PSI
#if 1
                if (tIdx == 0) gTimer.setTimePoint("online.recv.insertDone");

                // get the region of the base OTs that this thread should do.
                auto binStart = tIdx       * mBins.mBinCount / thrds.size();
                auto binEnd = (tIdx + 1) * mBins.mBinCount / thrds.size();
                auto otStart = binStart * 1;
                auto otEnd = binEnd * 1;

                PRNG prng(seed);
				
                u8 hashBuff[SHA1::HashSize];
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
                                ncoInput[j] = ncoInputBuff[j][inputIdx];
							
                            otRecv.encode(
								bIdx,      // input
                                ncoInput,             // input
                                recvMasks[inputIdx]); // output

							/*if (bIdx < 3 || (bIdx < mN && bIdx > mN-2))
								std::cout << "r-" << bIdx << ", " << inputIdx << ": " << recvMasks[inputIdx] << std::endl;*/
                        }
						else
                        {
                            otRecv.zeroEncode(bIdx);
                        }
						

                    }

                    otRecv.sendCorrection(chl, currentStepSize);
                }


                if (tIdx == 0) gTimer.setTimePoint("online.recv.recvMask");

                otRecv.check(chl);


                if (tIdx == 0) gTimer.setTimePoint("online.recv.sendMask");

                // all masks have been merged
                // this is the intersection that will be computed by this thread,
                // this will be merged into the overall list at the end.
                u64 localIntersection=-1;
				u64 mTheirBins_mMaxBinSize = 40;
				for (u64 bIdx = binStart; bIdx < binEnd;)
				{
                    u64 curStepSize = std::min(stepSize, binEnd - bIdx);

					MatrixView<u8> maskView;
					ByteStream maskBuffer;
					chl.recv(maskBuffer);
					maskView = maskBuffer.getMatrixView<u8>(maskSize);
					u64 numMasks = curStepSize * mTheirBins_mMaxBinSize;

					//if (maskView.size()[0] != numMasks)
					//	throw std::runtime_error("size not expedted");

					for (u64 stepIdx = 0; stepIdx < curStepSize; ++bIdx, ++stepIdx)
					{

						auto& bin = mBins.mBins[bIdx];
						u64 baseMaskIdx = stepIdx*mTheirBins_mMaxBinSize;

						if (!bin.isEmpty())
						{
							u64 inputIdx = bin.idx();
							auto myMask = recvMasks[inputIdx];
							
							for (u64 i = 0; i < mTheirBins_mMaxBinSize; ++i)
							{
								
								auto mask = maskView[baseMaskIdx + i];
								auto theirMask = ZeroBlock;
								memcpy(&theirMask, mask.data(), maskSize);

								
								//if (bIdx < 3 || (bIdx < mN && bIdx > mN - 2))
									//Log::out << theirMask << " "<< Log::endl;
								if (!memcmp((u8*)&myMask, &theirMask, maskSize))
								{
									Log::out << "inputIdx: " << inputIdx << Log::endl;
									Log::out << "myMask: " << myMask << Log::endl;
									Log::out << "theirMask: " << theirMask << " "<< Log::endl;
									localIntersection=inputIdx;

								}
							}
						}
					}
                }

                if (localIntersection!=-1)
                {
                    std::lock_guard<std::mutex> lock(mInsertMtx);
                    if (mIntersection.size())
                    {
                        mIntersection.push_back(localIntersection);
                    }
                }
                if (tIdx == 0) gTimer.setTimePoint("online.recv.done");

#endif
#pragma endregion
                //if (!tIdx)
                //    gTimer.setTimePoint("sendInput.done");
            });
        }

        // join the threads.
        for (u64 tIdx = 0; tIdx < thrds.size(); ++tIdx)
            thrds[tIdx].join();

        gTimer.setTimePoint("online.recv.exit");

        //std::cout << gTimer;
#endif
    }
#endif
}
#include "BarkOPRFSender.h"

#include "Crypto/Commit.h"
#include "Common/Log.h"
#include "Common/Log1.h"
#include "Common/Timer.h"
#include "Base/naor-pinkas.h"
#include "TwoChooseOne/IknpOtExtReceiver.h"
#include "TwoChooseOne/IknpOtExtReceiver.h"
namespace osuCrypto
{

    BarkOPRFSender::BarkOPRFSender()
    {
    }
    //const u64 BarkOPRFSender::hasherStepSize(128);


    BarkOPRFSender::~BarkOPRFSender()
    {
    }

    void BarkOPRFSender::init(u64 n, u64 statSec, u64 inputBitSize,
        Channel & chl0,
        NcoOtExtSender&  ots,
        block seed)
    {
        init(n, statSec, inputBitSize, { &chl0 }, ots, seed);
    }

    void BarkOPRFSender::init(u64 n, u64 statSec, u64 inputBitSize,
        const std::vector<Channel*>& chls,
        NcoOtExtSender& otSend,
        block seed)
    {
        mStatSecParam = statSec;
        mN = n;
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

		

        mBins.init(2,n, inputBitSize, mHashingSeed, statSec);

        //mPsis.resize(mBins.mBinCount);

        u64 perBinOtCount = 1;// mPsis[0].PsiOTCount(mBins.mMaxBinSize, mBins.mRepSize);
        u64 otCount = perBinOtCount * mBins.mBinCount;

        gTimer.setTimePoint("init.send.baseStart");

        if (otSend.hasBaseOts() == false)
        {
            // first do 128 public key OTs (expensive)
            std::array<std::array<block, 2>, gOtExtBaseOtCount> baseMsg;
            NaorPinkas base;
            base.send(baseMsg, mPrng, chl0, 2);


            // now extend these to enough recv OTs to seed the send Kco and the send Kos ot extension
            BitVector recvChoice(baseOtCount); recvChoice.randomize(mPrng);
            std::vector<block> recvBaseMsg(baseOtCount);
			IknpOtExtReceiver IknpRecv;
            IknpRecv.setBaseOts(baseMsg);
            IknpRecv.receive(recvChoice, recvBaseMsg, mPrng, chl0);

			std::cout << " - " << std::endl;
            // we now have a bunch of recv OTs, lets seed the NcoOtExtSender
            BitVector kcoSendBaseChoice; 
            kcoSendBaseChoice.copy(recvChoice, 0, baseOtCount);
            ArrayView<block> kcoSendBase(
                recvBaseMsg.begin(),
                recvBaseMsg.begin() + baseOtCount);
           
            otSend.setBaseOts(kcoSendBase, kcoSendBaseChoice);
        }

        gTimer.setTimePoint("init.send.extStart");

        mOtSends.resize(chls.size());


        auto sendRoutine = [&](u64 tIdx, u64 total, NcoOtExtSender& ots, Channel& chl)
        {
            auto start = (  tIdx     * mBins.mBinCount / total) ;
            auto end =   ((tIdx + 1) * mBins.mBinCount / total) ;

            ots.init(end - start);
        };


        u64 numThreads = chls.size()-1;

        std::vector<std::thread> thrds(numThreads);
        auto thrdIter = thrds.begin();
        auto chlIter = chls.begin() + 1;


        for (u64 i = 0; i < numThreads; ++i)
        {
            mOtSends[i+1] = std::move(otSend.split());

            *thrdIter++ = std::thread([&, i, chlIter]()
            {
                //std::cout << IoStream::lock << "s sendOt " << l << "  " << (**chlIter).getName() << std::endl << IoStream::unlock;
                sendRoutine(i+1, numThreads+1, *mOtSends[i+1], **chlIter);
            });
            ++chlIter;
        }


        mOtSends[0] = std::move(otSend.split());
        sendRoutine(0, numThreads+1, *mOtSends[0], chl0);

        for (auto& thrd : thrds)
            thrd.join();

        gTimer.setTimePoint("init.send.done");

    }


    void BarkOPRFSender::sendInput(std::vector<block>& inputs, Channel & chl)
    {
        sendInput(inputs, { &chl });
    }

    void BarkOPRFSender::sendInput(std::vector<block>& inputs, const std::vector<Channel*>& chls)
    {

        if (inputs.size() != mN)
            throw std::runtime_error(LOCATION);


		//TODO: double check
       // u64 maskSize = roundUpTo(mStatSecParam + 2 * std::log(mN) - 1, 8) / 8;
		u64 maskSize = 7;
        if (maskSize > sizeof(block))
            throw std::runtime_error("masked are stored in blocks, so they can exceed that size");


        std::vector<std::thread>  thrds(chls.size());
       // std::vector<std::thread>  thrds(1);        

		std::atomic<u32> remaining((u32)thrds.size()), remainingMasks((u32)thrds.size());
		std::promise<void> doneProm , maskProm;
		std::shared_future<void>
			doneFuture(doneProm.get_future()),
            maskFuture(maskProm.get_future());

        std::mutex mtx;

        std::vector<std::vector<block>> ncoInputBuff(mNcoInputBlkSize);
        std::vector<block> recvMasks(mN);

        for (u64 hashIdx = 0; hashIdx < ncoInputBuff.size(); ++hashIdx)
            ncoInputBuff[hashIdx].resize(inputs.size());


        std::vector<u64> maskPerm(mBins.mMaxBinSize);

        auto permSeed = mPrng.get<block>();

        std::promise<void> permProm;
        std::shared_future<void> permDone(permProm.get_future());

//        auto permThrd = std::thread([&]() {
//            PRNG prng(permSeed);
//			std::vector<u64> maskPerm(mBins.mMaxBinSize);
//            for (u64 i = 0; i < maskPerm.size(); ++i)
//                maskPerm[i] = i;
////TODO:
////   std::shuffle(maskPerm.begin(), maskPerm.end(), prng);
//            //u64 l, u32Max = (u32(-1));
//            //for (l = maskPerm.size(); l > u32Max; --l)
//            //{
//            //    u64 d = prng.get<u64>() % l;
//
//            //    u64 pi = maskPerm[l];
//            //    maskPerm[l] = maskPerm[d];
//            //    maskPerm[d] = pi;
//            //}
//            //for (l = maskPerm.size(); l > 1; --l)
//            //{
//
//            //    u32 d = prng.get<u32>() % l;
//
//            //    u64 pi = maskPerm[l];
//            //    maskPerm[l] = maskPerm[d];
//            //    maskPerm[d] = pi;
//            //}
//            permProm.set_value();
//        });
//
//
        
       
        
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
#pragma region Hashing				
                std::vector<AES> ncoInputHasher(mNcoInputBlkSize);
				//Log::out << "mHashingSeed: " << mHashingSeed << Log::endl;
                for (u64 i = 0; i < ncoInputHasher.size(); ++i)
                    ncoInputHasher[i].setKey(_mm_set1_epi64x(i) ^ mHashingSeed);

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
                    for (u64 j = 0; j < currentStepSize; ++j)
                    {
						ArrayView<u64> hashes(3);
						for (u64 k = 0; k < 3; ++k)
						{
							block& item = ncoInputBuff[k][i + j];
							u64 addr = *(u64*)&item % mBins.mBinCount;

							// implements phase. Note that we are doing very course phasing. 
							// At the byte level. This is good enough for use. Since we just 
							// need things tp be smaller than 76 bits.
						//	if (phaseShift == 3)
						//		ncoInputBuff[0][i + j] = _mm_srli_si128(item, 3);
						//	else// if (phaseShift <= 2)
						//		ncoInputBuff[0][i + j] = _mm_srli_si128(item, 2);

							std::lock_guard<std::mutex> lock(mBins.mMtx[addr]);
							mBins.mBins[addr].mIdx.emplace_back(i + j);
						//	std::cout << j<<"-" << k <<": " << *(u64*)&item << "\n";

							hashes[k] = *(u64*)&item;
							
						}

						//std::cout << j << ": " << hashes[0] << " " << hashes[1] << " " << hashes[2] << "\n";

						//if (j <2)
						//{
							
						//}

						
                    }
                }

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

				//mBins.print();
#pragma endregion


#pragma region maskCompute
#if 1
                if (tIdx == 0) gTimer.setTimePoint("online.send.insert");

                const u64 stepSize = 16;

                auto binStart = tIdx       * mBins.mBinCount / thrds.size();
                auto binEnd = (tIdx + 1) * mBins.mBinCount / thrds.size();

				auto otStart = binStart;// *mBins.mMaxBinSize;
				auto otEnd = binEnd;//;* mBins.mMaxBinSize;



                std::vector<u16> permutation(mBins.mMaxBinSize);
                for (size_t i = 0; i < permutation.size(); i++)
                    permutation[i] = (u16)i;

                u64 otIdx = 0;
                u64 maskIdx = otStart;
                std::vector<block> ncoInput(mNcoInputBlkSize);


                Buff buff;
                otIdx = 0;

              //  permDone.get();
                if (tIdx == 0) gTimer.setTimePoint("online.send.OT");


                for (u64 bIdx = binStart; bIdx < binEnd;)
                {

                    u64 currentStepSize = std::min(stepSize, binEnd - bIdx);
					
                    otSend.recvCorrection(chl, currentStepSize);					

					uPtr<Buff> sendMaskBuff(new Buff);
					sendMaskBuff->resize(currentStepSize * mBins.mMaxBinSize * maskSize);
					auto maskView = sendMaskBuff->getMatrixView<u8>(maskSize);


                    for (u64 stepIdx = 0; stepIdx < currentStepSize; ++bIdx, ++stepIdx)
                    {
						
                        auto& bin = mBins.mBins[bIdx];                        
						u64 baseMaskIdx = stepIdx* mBins.mMaxBinSize;

						std::vector<u64> maskPerm(mBins.mMaxBinSize);
						for (u64 i = 0; i < maskPerm.size(); ++i)
							maskPerm[i] = i;

                        for (u64 i = 0; i < bin.mIdx.size(); ++i)
                        {

                            u64 inputIdx = bin.mIdx[i];
							

                         
                                for (u64 j = 0; j < mNcoInputBlkSize; ++j)
                                {
                                    ncoInput[j] = ncoInputBuff[j][inputIdx];
                                }

                                block sendMask;

                                otSend.encode(
									bIdx, //each bin has 1 OT
                                    ncoInput,
                                    sendMask);

                                memcpy(
                                    maskView[baseMaskIdx + maskPerm[i]].data(),
                                    (u8*)&sendMask,
                                    maskSize);
								
								if (bIdx < 3 || (bIdx < mN && bIdx > mN - 2))
									std::cout << "s-"<<bIdx <<", "<< inputIdx << ": " << sendMask << std::endl;
                            }

					//	dummy mask
						for (u64 i = bin.mIdx.size(); i < mBins.mMaxBinSize ; ++i)
						{
							memcpy(
								maskView[baseMaskIdx + maskPerm[i]].data(),
								(u8*)&ZeroBlock, //make randome
								maskSize);
						}

						//if (tIdx == 0)
							
                    }
					chl.asyncSend(std::move(sendMaskBuff));
                }
                if (tIdx == 0) gTimer.setTimePoint("online.send.sendMask");


               
                otSend.check(chl);

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

}



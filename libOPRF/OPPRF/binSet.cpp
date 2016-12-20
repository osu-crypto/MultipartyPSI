#include "binSet.h"
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
	binSet::binSet()
    {
    }

	binSet::~binSet()
    {
    }

    void binSet::init(u64 myIdx, u64 nParties, u64 setSize, u64 statSecParam)
    {
		mMyIdx = myIdx;
		mParties = nParties;
		mN = setSize;
		mStatSecParam = statSecParam;

		//Hard-coding key for hash functions 
		mHashingSeed = _mm_set_epi64x(1, 1);

		mNcoInputBlkSize = 4;

		//mOpprfSends.resize(mParties- mMyIdx);
		//mOpprfRecvs.resize(mMyIdx);

	/*	std::vector<OPPRFSender> mOpprfSends(3);
		std::vector<OPPRFReceiver> mOpprfRecvs(3);*/

		mSimpleBins.init(mN);
		mCuckooBins.init(mN);
    }

  

	

	void binSet::hashing2Bins(std::vector<block>& inputs, int numThreads)
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


        std::vector<std::thread>  thrds(numThreads);
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

            thrds[tIdx] = std::thread([&, tIdx]()
            {

                if (tIdx == 0) gTimer.setTimePoint("online.recv.thrdStart");


                auto startIdx = tIdx     * mN / thrds.size();
                auto endIdx = (tIdx + 1) * mN / thrds.size();
#pragma region Hashing


                std::vector<AES> ncoInputHasher(mNcoInputBlkSize);
                for (u64 i = 0; i < ncoInputHasher.size(); ++i)
                    ncoInputHasher[i].setKey(_mm_set1_epi64x(i) ^ mHashingSeed);


                for (u64 i = startIdx; i < endIdx; i += 128)
                {
                    auto currentStepSize = std::min(u64(128), endIdx - i);

                    for (u64 hashIdx = 0; hashIdx < ncoInputHasher.size(); ++hashIdx)
                    {
                        ncoInputHasher[hashIdx].ecbEncBlocks(
							inputs.data() + i,
                            currentStepSize,
							mNcoInputBuff[hashIdx].data() + i);
                    }
					
					std::vector<block> tempMaskBuff(currentStepSize);
					std::vector<u64> tempIdxBuff(currentStepSize); 
					CuckooHasher1::Workspace w(tempMaskBuff.size());
					MatrixView<u64> hashes(currentStepSize,mCuckooBins.mParams.mNumHashes[0]);

                    for (u64 j = 0; j < currentStepSize; ++j)
                    {
						tempIdxBuff[j] = i + j;
						for (u64 k = 0; k <mCuckooBins.mParams.mNumHashes[0]; ++k)
						{
							hashes[j][k] = *(u64*)&mNcoInputBuff[k][i + j];
						}		                 					
					}

					mSimpleBins.insertBatch(tempIdxBuff, hashes);
					mCuckooBins.insertBatch(tempIdxBuff, hashes, w);
				
                }
				// block until all items have been inserted. the last to finish will set the promise...
				if (--insertRemaining)
					insertFuture.get();
				else
					insertProm.set_value();

				CuckooHasher1::Workspace stashW(mCuckooBins.mStashIdxs.size());
				MatrixView<u64> stashHashes(mCuckooBins.mStashIdxs.size(), mCuckooBins.mParams.mNumHashes[1]);

				for (u64 j = 0; j < mCuckooBins.mStashIdxs.size(); ++j)
				{
					for (u64 k = 0; k <mCuckooBins.mParams.mNumHashes[1]; ++k)
					{
						stashHashes[j][k] = *(u64*)&mNcoInputBuff[k][mCuckooBins.mStashIdxs[j]];
					}
				}
				mCuckooBins.insertStashBatch(mCuckooBins.mStashIdxs, stashHashes, stashW);

              




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

	

}
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
#include "Hashing/Hints.h"


//#define PRINT
namespace osuCrypto
{
		void  OPPRFReceiver::recvPlain(u64 IdxP, binSet& bins, const std::vector<Channel*>& chls)
	{

		// this is the online phase.
		gTimer.setTimePoint("online.recv.start");


		u32 numHashes = bins.mSimpleBins.mNumHashes[0] + bins.mSimpleBins.mNumHashes[1];
		if (bins.mMaskSize > sizeof(block))
			throw std::runtime_error("masked are stored in blocks, so they can exceed that size");




		std::vector<std::thread>  thrds(chls.size());
		// this mutex is used to guard inserting things into the intersection vector.
		std::mutex mInsertMtx;

		auto& chl = *chls[0];

		ByteStream maskBuffer;
		chl.recv(maskBuffer);

		auto maskBFView = maskBuffer.getMatrixView<u8>(bins.mMaskSize);

		//std::cout << "\nr[" << IdxP << "]-maskBFView.size() " << maskBFView.size()[0] << "\n";
		//	std::cout << "\nr[" << IdxP << "]-mBfBitCount " << mBfSize << "\n";
		//std::cout << "totalMask: " << totalMask << "\n";

		//std::cout << "\nr[" << IdxP << "]-maskBFView[1][3]" << maskBFView[1][3] << "\n";

#if 1
		

		for (u64 tIdx = 0; tIdx < thrds.size(); ++tIdx)
		{
			auto seed = mPrng.get<block>();
			thrds[tIdx] = std::thread([&, tIdx, seed]()
			{
				if (tIdx == 0) gTimer.setTimePoint("online.recv.thrdStart");

				auto& chl = *chls[tIdx];
				const u64 stepSize = 16;

				if (tIdx == 0) gTimer.setTimePoint("online.recv.recvShare");

				//2 type of bins: normal bin in inital step + stash bin
				for (auto bIdxType = 0; bIdxType < 2; bIdxType++)
				{
					auto binCountRecv = bins.mCuckooBins.mBinCount[bIdxType];

					u64 binStart, binEnd;
					if (bIdxType == 0)
					{
						binStart = tIdx       * binCountRecv / thrds.size();
						binEnd = (tIdx + 1) * binCountRecv / thrds.size();
					}
					else
					{
						binStart = tIdx       * binCountRecv / thrds.size() + bins.mCuckooBins.mBinCount[0];
						binEnd = (tIdx + 1) * binCountRecv / thrds.size() + bins.mCuckooBins.mBinCount[0];
					}


					for (u64 bIdx = binStart; bIdx < binEnd;)
					{
						u64 curStepSize = std::min(stepSize, binEnd - bIdx);

						for (u64 stepIdx = 0; stepIdx < curStepSize; ++bIdx, ++stepIdx)
						{
							auto& bin = bins.mCuckooBins.mBins[bIdx];
							if (!bin.isEmpty())
							{
								u64 inputIdx = bin.idx();
								u64 hIdx = bin.hashIdx();

								auto theirBFMask = ZeroBlock;
								memcpy(&theirBFMask, maskBFView[hIdx*mN + inputIdx].data(), bins.mMaskSize);

								/*if (inputIdx == 0)
								{
								std::cout << "inputIdx[" << inputIdx << "]-hIdx[" << hIdx << "]-OPRF" << bin.mValOPRF[IdxP];
								std::cout << "\n----" << blkY << std::endl;
								}*/
								
								if (!memcmp((u8*)&bin.mValOPRF[IdxP], &theirBFMask, bins.mMaskSize))
								{
									mIntersection.push_back(inputIdx);
								}

							}
						}
					}
				}


			});
			//	if (tIdx == 0) gTimer.setTimePoint("online.recv.done");
		}
		// join the threads.
		for (auto& thrd : thrds)
			thrd.join();

#endif // 0


	}
}
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
#if 1
		// this is the online phase.
		gTimer.setTimePoint("online.recv.start");


		std::vector<std::thread>  thrds(chls.size());
		//  std::vector<std::thread>  thrds(1);

		// we use 5 unordered_maps, we put the mask to the corresponding unordered_map
		//that indicates of the hash function index 0,1,2,3,4.
		std::array<std::unordered_map<u32, std::pair<block, u64>>, 5> localMasks;
		//store the masks of elements that map to bin by h0
		localMasks[0].reserve(mN); //upper bound of # mask
		localMasks[1].reserve(mN); //upper bound of # mask
		localMasks[2].reserve(mN); //upper bound of # mask
		localMasks[3].reserve(mN); //upper bound of # mask
		localMasks[4].reserve(mN); //upper bound of # mask

		// fr each thread, spawn it.
		for (u64 tIdx = 0; tIdx < thrds.size(); ++tIdx)
		{
			auto seed = mPrng.get<block>();
			thrds[tIdx] = std::thread([&, tIdx, seed]()
			{

				if (tIdx == 0) gTimer.setTimePoint("online.recv.thrdStart");



				auto& chl = *chls[tIdx];

				if (tIdx == 0) gTimer.setTimePoint("online.recv.insertDone");

				const u64 stepSize = 16;

				std::vector<block> ncoInput(bins.mNcoInputBlkSize);

#if 1
#pragma region compute Recv Bark-OPRF

				//####################
				//#######Recv role
				//####################
				auto& otRecv = *mOtRecvs[tIdx];

				auto otCountRecv = bins.mCuckooBins.mBins.size();
				// get the region of the base OTs that this thread should do.
				auto binStart = tIdx       * otCountRecv / thrds.size();
				auto binEnd = (tIdx + 1) * otCountRecv / thrds.size();



				for (u64 bIdx = binStart; bIdx < binEnd;)
				{
					u64 currentStepSize = std::min(stepSize, binEnd - bIdx);

					for (u64 stepIdx = 0; stepIdx < currentStepSize; ++bIdx, ++stepIdx)
					{
						auto& bin = bins.mCuckooBins.mBins[bIdx];

						if (!bin.isEmpty())
						{
							u64 inputIdx = bin.idx();
							u64 hIdx = bin.hashIdx();

							for (u64 j = 0; j < ncoInput.size(); ++j)
								ncoInput[j] = bins.mNcoInputBuff[j][inputIdx];

							otRecv.encode(
								bIdx,      // input
								ncoInput,             // input
								bin.mValOPRF[IdxP]); // output

							localMasks[hIdx].emplace(*(u32*)&bin.mValOPRF[IdxP], std::pair<block, u64>(bin.mValOPRF[IdxP], inputIdx));
						}
						else
							otRecv.zeroEncode(bIdx);
					}
					otRecv.sendCorrection(chl, currentStepSize);
				}

				if (tIdx == 0) gTimer.setTimePoint("online.recv.otRecv.finalOPRF");



#pragma endregion
#endif

				otRecv.check(chl);
			});
		}

		// join the threads.
		for (u64 tIdx = 0; tIdx < thrds.size(); ++tIdx)
			thrds[tIdx].join();

		gTimer.setTimePoint("online.recv.exit");

		//std::cout << gTimer;
#endif

		auto& chl = *chls[0];

		

		//std::cout << "\nr[" << IdxP << "]-maskBFView.size() " << maskBFView.size()[0] << "\n";
		//	std::cout << "\nr[" << IdxP << "]-mBfBitCount " << mBfSize << "\n";
		//std::cout << "totalMask: " << totalMask << "\n";

		//std::cout << "\nr[" << IdxP << "]-maskBFView[1][3]" << maskBFView[1][3] << "\n";

#if 1
		u32 numHashes = bins.mSimpleBins.mNumHashes[0] + bins.mSimpleBins.mNumHashes[1];

		ByteStream maskBuffer1, maskBuffer2, maskBuffer3, maskBuffer4, maskBuffer5;
		chl.recv(maskBuffer1);
		chl.recv(maskBuffer2);
		chl.recv(maskBuffer3);
		chl.recv(maskBuffer4);
		chl.recv(maskBuffer5);
		std::vector<MatrixView<u8>> maskBFView(numHashes);
		for (u64 i = 0; i < numHashes; i++)
		{
			if (i == 0)
				maskBFView[i] = maskBuffer1.getMatrixView<u8>(bins.mMaskSize);
			else if (i == 1)
				maskBFView[i] = maskBuffer2.getMatrixView<u8>(bins.mMaskSize);
			else if (i == 2)
				maskBFView[i] = maskBuffer3.getMatrixView<u8>(bins.mMaskSize);
			else if (i == 3)
				maskBFView[i] = maskBuffer4.getMatrixView<u8>(bins.mMaskSize);
			else if (i == 4)
				maskBFView[i] = maskBuffer5.getMatrixView<u8>(bins.mMaskSize);

		//	std::cout << "\nr[" << IdxP << "]-maskBFView.size() " << maskBFView[i].size()[0] << "\n";

		}

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

								/*	auto theirBFMask = ZeroBlock;
									memcpy(&theirBFMask, maskBFView[hIdx][inputIdx].data(), bins.mMaskSize);
									Log::out << "theirBFMask: " << *(u32*)maskBFView[hIdx][inputIdx].data() << "\n";
									Log::out << "myBFMask: " << *(u32*)&bin.mValOPRF[IdxP] << "\n";

									Log::out << "theirBFMask: " << theirBFMask << "\n";
									Log::out << "myBFMask: " << bin.mValOPRF[IdxP] << "\n";
									u64 a = *(u32*)&bin.mValOPRF[IdxP];
									Log::out << "myBFMask: " << a << "\n";

									u32 a1, a2;
									memcpy(&a1, maskBFView[hIdx][inputIdx].data(), sizeof(u32));
									memcpy(&a2, &bin.mValOPRF[IdxP], sizeof(u32));
									Log::out << "a1: " << a1 << "\n";
									Log::out << "a2: " << a2 << "\n";*/



									auto& msk = *(u32*)maskBFView[hIdx][inputIdx].data();
									auto match = localMasks[hIdx].find(msk);
									if (match != localMasks[hIdx].end())
									{
										//auto theirBFMask = ZeroBlock;
										if (memcmp(maskBFView[hIdx][inputIdx].data(), &match->second.first, bins.mMaskSize) == 0) // check full mask
										{
											mIntersection.push_back(match->second.second);
											//Log::out << "#id: " << match->second.second << Log::endl;
										}
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
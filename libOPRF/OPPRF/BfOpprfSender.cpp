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

	void OPPRFSender::sendBFBased(u64 IdxP, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls)
	{
		if (plaintexts.size() != mN)
			throw std::runtime_error(LOCATION);

		u32 numHashes = bins.mSimpleBins.mNumHashes[0] + bins.mSimpleBins.mNumHashes[1];


		//our BF: y-oprf(x)=\xor hashBF_i(x)
		//each x has 5 diffirent values oprf1(x),...,oprf5(x)
		//our BF is an array of sized 40*|X|*ln(2)
		//each array has 5*bins.mMaskSize 
		//which presented as y-oprf1(x)||y-oprf2(x)||...||y-oprf5(x)

		mBfSize = mNumBFhashs * mN * std::log2(std::exp(1.0));


		//bins.mMaskSize = roundUpTo(mStatSecParam + 2 * std::log2(mN), 8) / 8;

		if (bins.mMaskSize > sizeof(block))
			throw std::runtime_error("masked are stored in blocks, so they can exceed that size");

		std::vector<std::thread>  thrds(chls.size());
		// std::vector<std::thread>  thrds(1);        


		uPtr<Buff> sendMaskBuff(new Buff);
		sendMaskBuff->resize(mBfSize* bins.mMaskSize*numHashes);
		auto maskBFView = sendMaskBuff->getMatrixView<u8>(bins.mMaskSize);

		//y-oprf1(x)||y-oprf2(x)||...||y-oprf5(x)
		std::vector<std::vector<block>> GarbleBF(numHashes);
		for (u64 hIdx = 0; hIdx < GarbleBF.size(); ++hIdx)
		{
			GarbleBF[hIdx].resize(mBfSize);
		}


		gTimer.setTimePoint("online.send.spaw");

	
		std::vector<block> hashs1(bins.mXsets.size());
		std::vector<block> hashs2(bins.mXsets.size());
		mBFHasher[0].ecbEncBlocks(bins.mXsets.data(), bins.mXsets.size(), (block*)hashs1.data());
		mBFHasher[1].ecbEncBlocks(bins.mXsets.data(), bins.mXsets.size(), (block*)hashs2.data());


		for (u64 tIdx = 0; tIdx < thrds.size(); ++tIdx)
		{
			auto seed = mPrng.get<block>();
			thrds[tIdx] = std::thread([&, tIdx, seed]() {


				PRNG prng(seed);

				if (tIdx == 0) gTimer.setTimePoint("online.send.thrdStart");

				auto& chl = *chls[tIdx];
				const u64 stepSize = 16;

#pragma region sendShare
#if 1
				if (tIdx == 0) gTimer.setTimePoint("online.send.sendShare");

				u64 idxStart, idxEnd;

				idxStart = tIdx       * mN / thrds.size();
				idxEnd = (tIdx + 1) * mN / thrds.size();




				if (tIdx == 0) gTimer.setTimePoint("online.send.masks.init.step");

				for (u64 inputIdx = idxStart; inputIdx < idxEnd;)
				{
					u64 currentStepSize = std::min(stepSize, idxEnd - inputIdx);

					for (u64 stepIdx = 0; stepIdx < currentStepSize; ++inputIdx, ++stepIdx)
					{
						u64 baseMaskIdx = stepIdx;
						int MaskIdx = 0;

						std::set<u64> idxs;


						u64& idx1 = *(u64*)&hashs1[inputIdx];
						idx1 %= mBfSize;
						idxs.emplace(idx1);
						//std::cout << "idx1" << idx1 << "\n";

						u64& idx2 = *(u64*)&hashs2[inputIdx];
						idx2 %= mBfSize;
						idxs.emplace(idx2);
						//std::cout << "idx2" << idx2 << "\n";

					

						//normal BF presented by one bit
						for (u64 BFhashIdx = 2; BFhashIdx < mBFHasher.size(); ++BFhashIdx)
						{
							/*block hashOut = mBFHasher[BFhashIdx].ecbEncBlock(bins.mXsets[inputIdx]);
							u64& idx = *(u64*)&hashOut;
							idx %= mBfSize;*/

							u64 idx = idx1 + BFhashIdx * idx2;
							idx %= mBfSize;
							idxs.emplace(idx);
						}

						//GBF
						for (u64 hIdx = 0; hIdx < numHashes; ++hIdx)
						{
							block sum = ZeroBlock;
							u64 firstFreeIdx(-1);
							for (auto idx : idxs)
							{
								if (eq(GarbleBF[hIdx][idx], ZeroBlock))
								{
									if (firstFreeIdx == u64(-1))
									{
										firstFreeIdx = idx;
										//	std::cout << "firstFreeIdx: " << firstFreeIdx << std::endl;
									}
									else
									{
										GarbleBF[hIdx][idx] = mPrng.get<block>();
										memcpy(maskBFView[hIdx*mBfSize + idx].data(), (u8*)&GarbleBF[hIdx][idx], bins.mMaskSize);
										//	std::cout << garbledBF[idx] <<"\n";
										sum = sum ^ GarbleBF[hIdx][idx];
										//std::cout << idx << " " << maskBFView[idx] << std::endl;
									}
								}
								else
								{
									sum = sum ^ GarbleBF[hIdx][idx];
									//	std::cout << idx << " " << maskBFView[idx] << std::endl;
								}
							}

							GarbleBF[hIdx][firstFreeIdx] = sum^plaintexts[inputIdx] ^ bins.mSimpleBins.mOprfs[IdxP][inputIdx][hIdx];
							memcpy(maskBFView[hIdx*mBfSize + firstFreeIdx].data(), (u8*)&GarbleBF[hIdx][firstFreeIdx], bins.mMaskSize);


							/*	if (inputIdx == 0)
							{
							block y = plaintexts[inputIdx] ^ bins.mSimpleBins.mOprfs[IdxP][inputIdx][hIdx];

							std::cout << "inputIdx[" << inputIdx << "]-hIdx[" << hIdx << "]-OPRF" << bins.mSimpleBins.mOprfs[IdxP][inputIdx][hIdx];
							std::cout << "\n----" << y << std::endl;
							}*/

						}

					}

				}



				if (tIdx == 0) gTimer.setTimePoint("online.compute x y");
#endif
#pragma endregion

			});
		}

		for (auto& thrd : thrds)
			thrd.join();

		for (u64 hIdx = 0; hIdx < numHashes; ++hIdx)
		{
			for (u64 i = 0; i < GarbleBF[hIdx].size(); ++i)
			{
				if (eq(GarbleBF[hIdx][i], ZeroBlock))
				{

					GarbleBF[hIdx][i] = mPrng.get<block>();
					memcpy(maskBFView[hIdx*mBfSize + i].data(), (u8*)&GarbleBF[hIdx][i], bins.mMaskSize);

				}
			}
		}

		//std::cout << "\ns[" << IdxP << "]-maskBFView.size() " << maskBFView.size()[0] << "\n";
		//std::cout << "\ns[" << IdxP << "]-mBfSize " << mBfSize << "\n";
		//std::cout << "\ns[" << IdxP << "]-mMaskSize " << bins.mMaskSize << "\n";


		std::cout << "s[" << IdxP << "]-dataSent(bytes)" << maskBFView.size()[0] * maskBFView.size()[1] << "----------\n";

		//std::cout << "\ns[" << IdxP << "]-arrayMask[1][3]" << arrayMask[1][3] << "\n";


		auto& chl = *chls[0];
		chl.asyncSend(std::move(sendMaskBuff));


	}

		void  OPPRFSender::recvBFBased(u64 IdxP, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls)
	{

		// this is the online phase.
		gTimer.setTimePoint("online.recv.start");


		u32 numHashes = bins.mSimpleBins.mNumHashes[0] + bins.mSimpleBins.mNumHashes[1];

		//our BF: y-oprf(x)=\xor hashBF_i(x)
		//each x has 5 diffirent values oprf1(x),...,oprf5(x)
		//our BF is an array of sized 40*|X|*ln(2)
		//each array has 5*bins.mMaskSize 
		//which presented as y-oprf1(x)||y-oprf2(x)||...||y-oprf5(x)

		mBfSize = mNumBFhashs * mN * std::log2(std::exp(1.0));

		//bins.mMaskSize = roundUpTo(mStatSecParam + 2 * std::log2(mN), 8) / 8;


		//u64 mMaskSize = sizeof(block);

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

								block blkY = ZeroBlock;


								for (u64 hashIdx = 0; hashIdx < mBFHasher.size(); ++hashIdx)
								{
									block hashOut = mBFHasher[hashIdx].ecbEncBlock(bins.mXsets[inputIdx]);
									u64& idx = *(u64*)&hashOut;
									idx %= mBfSize;
									auto theirBFMask = ZeroBlock;
									memcpy(&theirBFMask, maskBFView[hIdx*mBfSize + idx].data(), bins.mMaskSize);
									/*if(hIdx==1&&idx==3)
									std::cout << "\nr[" << IdxP << "]-arrayMask[1][3]" << theirBFMask << "\n";*/

									blkY = blkY ^ theirBFMask;
								}

								/*if (inputIdx == 0)
								{
								std::cout << "inputIdx[" << inputIdx << "]-hIdx[" << hIdx << "]-OPRF" << bin.mValOPRF[IdxP];
								std::cout << "\n----" << blkY << std::endl;
								}*/
								plaintexts[inputIdx] = bin.mValOPRF[IdxP] ^ blkY;
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



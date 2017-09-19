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
#include "Parameters.h"

//#define PRINT
namespace osuCrypto
{

	void  OPPRFReceiver::recvSSPolyBased(u64 IdxP, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls)
	{

		// this is the online phase.
		gTimer.setTimePoint("online.recv.start");


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

				auto& chl = *chls[tIdx];
				//const u64 stepSize = 16;

				if (tIdx == 0) gTimer.setTimePoint("online.recv.recvShare");

				//2 type of bins: normal bin in inital step + stash bin

				for (auto bIdxType = 0; bIdxType < 2; bIdxType++)
				{
					//bins.mMaskSize = roundUpTo(mStatSecParam + std::log2(bins.mSimpleBins.mMaxBinSize[bIdxType]), 8) / 8;
					BaseOPPRF poly;
					poly.poly_init(bins.mMaskSize);

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



					//use the params of the simple hashing as their params
					u64 mTheirBins_mMaxBinSize = bins.mSimpleBins.mMaxBinSize[bIdxType];

					for (u64 bIdx = binStart; bIdx < binEnd;)
					{
						u64 curStepSize = std::min(stepSize, binEnd - bIdx);

						MatrixView<u8> maskView;
						ByteStream maskBuffer;
						chl.recv(maskBuffer);

						maskView = maskBuffer.getMatrixView<u8>(mTheirBins_mMaxBinSize*bins.mMaskSize);

						if (maskView.size()[0] != curStepSize)
							throw std::runtime_error("size not expedted");

						for (u64 stepIdx = 0; stepIdx < curStepSize; ++bIdx, ++stepIdx)
						{

							auto& bin = bins.mCuckooBins.mBins[bIdx];
							if (!bin.isEmpty())
							{
								bin.mCoeffs[IdxP].resize(mTheirBins_mMaxBinSize);

								u64 baseMaskIdx = stepIdx;

								u64 inputIdx = bin.idx();

								//compute p(x*)

								for (u64 i = 0; i < mTheirBins_mMaxBinSize; i++)
								{
									memcpy(&bin.mCoeffs[IdxP][i], maskView[baseMaskIdx].data() + i*bins.mMaskSize, bins.mMaskSize);

									/*if (bIdx == 0 && i==3)
									{
									Log::out << "r["<< IdxP<<"]-coeffs[" << i << "] #" << bin.mCoeffs[IdxP][i] << Log::endl;

									}*/
								}

								block blkY;
								poly.evalPolynomial(bin.mCoeffs[IdxP], bin.mValOPRF[IdxP], blkY);

								plaintexts[inputIdx] = bin.mValOPRF[IdxP] ^ blkY;

								/*if (bIdx == 0)
								{
								std::cout << "r["<<IdxP<<"]-bin.mValOPRF[" << bIdx << "] " << bin.mValOPRF[IdxP];
								std::cout << "-----------" << blkY << std::endl;
								}*/

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



	}
	void OPPRFReceiver::recvFullPolyBased(u64 IdxP, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls)
	{

		// this is the online phase.
		gTimer.setTimePoint("online.recv.start");


		u32 numHashes = bins.mSimpleBins.mNumHashes[0] + bins.mSimpleBins.mNumHashes[1];

		//bins.mMaskSize = roundUpTo(mStatSecParam + 2 * std::log2(mN), 8) / 8;

		if (bins.mMaskSize > sizeof(block))
			throw std::runtime_error("masked are stored in blocks, so they can exceed that size");


		std::vector<std::thread>  thrds(chls.size());
		// this mutex is used to guard inserting things into the intersection vector.
		std::mutex mInsertMtx;

		auto& chl = *chls[0];

		ByteStream maskBuffer;
		chl.recv(maskBuffer);
		BaseOPPRF b;
		b.poly_init(bins.mMaskSize);
		NTL::GF2E e;
		std::vector<NTL::GF2EX> polynomial(numHashes);

		auto maskView = maskBuffer.getMatrixView<u8>(bins.mMaskSize);

		//std::cout << "maskView.size()" << maskView.size()[0] << "\n";
		//std::cout << "totalMask: " << totalMask << "\n";

		//std::cout << "\nr[" << IdxP << "]-coeffs[3]" << maskView[3] << "\n";

		block blkCoff;
		for (u64 hIdx = 0; hIdx < numHashes; ++hIdx)
			for (u64 i = 0; i < mN; ++i)
			{

				memcpy(&blkCoff, maskView[hIdx*mN + i].data(), bins.mMaskSize);
				/*if(i==3 && hIdx==1)
				std::cout << "\nr[" << IdxP << "]-coeffs][1][3]" << blkCoff << "\n";*/
				b.GF2EFromBlock(e, blkCoff, bins.mMaskSize);
				NTL::SetCoeff(polynomial[hIdx], i, e); //build res_polynomial
			}

#if 1

		for (u64 tIdx = 0; tIdx < thrds.size(); ++tIdx)
		{
			auto seed = mPrng.get<block>();
			thrds[tIdx] = std::thread([&, tIdx, seed]()
			{
				if (tIdx == 0) gTimer.setTimePoint("online.recv.thrdStart");

				auto& chl = *chls[tIdx];
				//const u64 stepSize = 16;

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
								block blkY;
								b.GF2EFromBlock(e, bins.mXsets[inputIdx], bins.mMaskSize);
								e = NTL::eval(polynomial[hIdx], e); //get y=f(x) in GF2E
								b.BlockFromGF2E(blkY, e, bins.mMaskSize);

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
	void OPPRFReceiver::sendSSPolyBased(u64 IdxP, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls)
	{
		if (plaintexts.size() != mN)
			throw std::runtime_error(LOCATION);

		std::vector<std::thread>  thrds(chls.size());
		// std::vector<std::thread>  thrds(1);        



		std::mutex mtx;
		NTL::vec_GF2E x; NTL::vec_GF2E y;
		NTL::GF2E e;

		gTimer.setTimePoint("online.send.spaw");

		for (u64 tIdx = 0; tIdx < thrds.size(); ++tIdx)
		{
			auto seed = mPrng.get<block>();
			thrds[tIdx] = std::thread([&, tIdx, seed]() {

				PRNG prng(seed);

				if (tIdx == 0) gTimer.setTimePoint("online.send.thrdStart");

				auto& chl = *chls[tIdx];
				//const u64 stepSize = 16;

#pragma region sendShare
#if 1
				if (tIdx == 0) gTimer.setTimePoint("online.send.sendShare");

				//2 type of bins: normal bin in inital step + stash bin

				for (auto bIdxType = 0; bIdxType < 2; bIdxType++)
				{

					BaseOPPRF mPoly;
					//bins.mMaskSize = roundUpTo(mStatSecParam + std::log2(bins.mSimpleBins.mMaxBinSize[bIdxType]), 8) / 8;
					mPoly.poly_init(bins.mMaskSize);

					auto binCountSend = bins.mSimpleBins.mBinCount[bIdxType];
					u64 binStart, binEnd;
					if (bIdxType == 0)
					{
						binStart = tIdx       * binCountSend / thrds.size();
						binEnd = (tIdx + 1) * binCountSend / thrds.size();

					}
					else
					{
						binStart = tIdx       * binCountSend / thrds.size() + bins.mSimpleBins.mBinCount[0];
						binEnd = (tIdx + 1) * binCountSend / thrds.size() + bins.mSimpleBins.mBinCount[0];

					}


					if (tIdx == 0) gTimer.setTimePoint("online.send.masks.init.step");

					for (u64 bIdx = binStart; bIdx < binEnd;)
					{
						u64 currentStepSize = std::min(stepSize, binEnd - bIdx);
						uPtr<Buff> sendMaskBuff(new Buff);
						sendMaskBuff->resize(currentStepSize * (bins.mSimpleBins.mMaxBinSize[bIdxType] * bins.mMaskSize));
						auto maskView = sendMaskBuff->getMatrixView<u8>(bins.mSimpleBins.mMaxBinSize[bIdxType] * bins.mMaskSize);

						for (u64 stepIdx = 0; stepIdx < currentStepSize; ++bIdx, ++stepIdx)
						{
							//Log::out << "sBin #" << inputIdx << Log::endl;

							auto& bin = bins.mSimpleBins.mBins[bIdx];
							u64 baseMaskIdx = stepIdx;
							int MaskIdx = 0;

							//	Log::out << "bin.mIdx[" << inputIdx << "]: " <<  Log::endl;

							if (bin.mIdx.size() > 0)
							{

								//get y[i]
								std::vector<block> setY(bin.mIdx.size());
								for (u64 i = 0; i < bin.mIdx.size(); ++i)
								{
									u64 inputIdx = bin.mIdx[i];
									//NOTE that it is fine to compute p(oprf(x[i]))=y[i] as long as receiver reconstruct y*=p(oprf(x*))

									setY[i] = plaintexts[inputIdx] ^ bin.mValOPRF[IdxP][i];
									/*	if (bIdx == 0)
									{
									std::cout << "s bin.mValOPRF[" << bIdx << "] " << bin.mValOPRF[IdxP][i];
									std::cout << "-----------" << setY[i] << std::endl;
									}*/
								}

								std::vector<block> coeffs;
								//computes coefficients (in blocks) of p such that p(x[i]) = y[i]
								//NOTE that it is fine to compute p(oprf(x[i]))=y[i] as long as receiver reconstruct y*=p(oprf(x*))

								mPoly.getBlkCoefficients(bins.mSimpleBins.mMaxBinSize[bIdxType],
									bin.mValOPRF[IdxP], setY, coeffs);

								//if (bIdx == 0)
								//{
								//	//Log::out << "coeffs.size(): " << coeffs.size()<< Log::endl;

								//	for (u64 i = 0; i < bins.mSimpleBins.mMaxBinSize[bIdxType]; ++i)
								//		if (i == 3)
								//			Log::out << IdxP << "s-coeffs[" << i << "] #" << coeffs[i] << Log::endl;
								//}

								//it already contain a dummy item
								for (u64 i = 0; i < bins.mSimpleBins.mMaxBinSize[bIdxType]; ++i)
								{
									memcpy(
										maskView[baseMaskIdx].data() + i* bins.mMaskSize,
										(u8*)&coeffs[i],
										bins.mMaskSize);
								}

							}
							else //pad all dummy
							{
								for (u64 i = 0; i < bins.mSimpleBins.mMaxBinSize[bIdxType]; ++i)
								{
									memcpy(
										maskView[baseMaskIdx].data() + i* bins.mMaskSize,
										(u8*)&ZeroBlock,  //make randome
										bins.mMaskSize);
								}
							}
						}

						chl.asyncSend(std::move(sendMaskBuff));

					}
				}

				if (tIdx == 0) gTimer.setTimePoint("online.send.finalMask");
#endif
#pragma endregion

			});
		}

		for (auto& thrd : thrds)
			thrd.join();

	}
	void OPPRFReceiver::sendFullPolyBased(u64 IdxP, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls)
	{
		if (plaintexts.size() != mN)
			throw std::runtime_error(LOCATION);

		//bins.mMaskSize = roundUpTo(mStatSecParam + 2 * std::log2(mN), 8) / 8;

		if (bins.mMaskSize > sizeof(block))
			throw std::runtime_error("masked are stored in blocks, so they can exceed that size");

		std::vector<std::thread>  thrds(chls.size());
		// std::vector<std::thread>  thrds(1);        

		u32 numHashes = bins.mSimpleBins.mNumHashes[0] + bins.mSimpleBins.mNumHashes[1];

		std::mutex mtx;
		std::vector<NTL::vec_GF2E> vec_GF2E_X(numHashes);
		std::vector<NTL::vec_GF2E> vec_GF2E_Y(numHashes);
		std::vector<u64> size_vec_GF2E_X(numHashes);
		NTL::GF2E e;
		BaseOPPRF base_poly;
		base_poly.poly_init(bins.mMaskSize);

		gTimer.setTimePoint("online.send.spaw");

		for (u64 tIdx = 0; tIdx < thrds.size(); ++tIdx)
		{
			auto seed = mPrng.get<block>();
			thrds[tIdx] = std::thread([&, tIdx, seed]() {

				PRNG prng(seed);

				if (tIdx == 0) gTimer.setTimePoint("online.send.thrdStart");

				auto& chl = *chls[tIdx];
				//const u64 stepSize = 16;

#pragma region sendShare
#if 1
				if (tIdx == 0) gTimer.setTimePoint("online.send.sendShare");

				//2 type of bins: normal bin in inital step + stash bin
				//auto binCountSend = bins.mSimpleBins.mBinCount[bIdxType];

				u64 idxStart, idxEnd; //by mXset 

				idxStart = tIdx       * mN / thrds.size();
				idxEnd = (tIdx + 1) * mN / thrds.size();


				for (u64 inputIdx = idxStart; inputIdx < idxEnd;)
				{
					u64 currentStepSize = std::min(stepSize, idxEnd - inputIdx);

					for (u64 stepIdx = 0; stepIdx < currentStepSize; ++inputIdx, ++stepIdx)
					{

						u64 baseMaskIdx = stepIdx;
						int MaskIdx = 0;

						//compute p((x[i]))=y[i]-(oprf(x[i])) by BFhashIdx

						for (u64 hIdx = 0; hIdx < numHashes; ++hIdx)
						{

							block y = plaintexts[inputIdx] ^ bins.mSimpleBins.mOprfs[IdxP][inputIdx][hIdx];
							base_poly.GF2EFromBlock(e, y, bins.mMaskSize);

							//TODO: current test is single thread, make safe when running multi-thread
							vec_GF2E_Y[hIdx].append(e);

							base_poly.GF2EFromBlock(e, bins.mXsets[inputIdx], bins.mMaskSize);

							//TODO: current test is single thread, make safe when running multi-thread
							vec_GF2E_X[hIdx].append(e);
							size_vec_GF2E_X[hIdx]++;

							/*if (inputIdx == 0)
							{
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
		/*std::cout << bins.mN << " - " << bins.mSimpleBins.mNumHashes[0] << " " << bins.mSimpleBins.mNumHashes[1] << "\n";*/



		//ADDING DUMMY
		//because 2 h(x1) and h(x2) might have the same value
		for (u64 hIdx = 0; hIdx < numHashes; ++hIdx)
		{
			/*std::cout << "bins.mN - size_vec_GF2E_X[" << hIdx << "]"
			<< bins.mN - size_vec_GF2E_X[hIdx] << "\n";*/

			for (u32 i = 0; i < bins.mN - size_vec_GF2E_X[hIdx]; i++)
			{
				NTL::random(e);
				vec_GF2E_X[hIdx].append(e);
				NTL::random(e);
				vec_GF2E_Y[hIdx].append(e);
			}
		}


		//get Blk Coefficients and send it to receiver
		std::vector<std::vector<block>> coeffs(numHashes);
		//computes coefficients (in blocks) of p such that p(x[i]) = y[i]
		//NOTE that it is fine to compute p(oprf(x[i]))=y[i] as long as receiver reconstruct y*=p(oprf(x*))

		for (u64 hIdx = 0; hIdx < numHashes; ++hIdx)
		{
			//	std::cout << "getBlkCoefficients " << hIdx <<"start \n";
			base_poly.getBlkCoefficients(vec_GF2E_X[hIdx], vec_GF2E_Y[hIdx], coeffs[hIdx]);
			//std::cout << "getBlkCoefficients " << hIdx << "end \n";
		}
		//	std::cout << "coeffs.size()" << coeffs.size() << "\n";		
		//	std::cout << "totalMask: " << totalMask << "\n";


		uPtr<Buff> sendMaskBuff(new Buff);
		sendMaskBuff->resize(bins.mN*numHashes* bins.mMaskSize);
		auto maskView = sendMaskBuff->getMatrixView<u8>(bins.mMaskSize);

#if 1

		//it already contain a dummy item
		for (u64 hIdx = 0; hIdx < numHashes; ++hIdx)
		{
			//std::cout << "coeffs["<<hIdx<<"].size()"<<coeffs[hIdx].size()<<"\n";
			for (u64 i = 0; i < coeffs[hIdx].size(); ++i)
			{
				memcpy(
					maskView[hIdx*mN + i].data(),
					(u8*)&coeffs[hIdx][i],  //make randome
											//(u8*)&ZeroBlock,  //make randome
					bins.mMaskSize);
			}
		}
		//std::cout << "s[" << IdxP << "]-coeffs[1][3]" << coeffs[1][3] << "\n";


		auto& chl = *chls[0];
		chl.asyncSend(std::move(sendMaskBuff));

#endif // 0


	}
	
}
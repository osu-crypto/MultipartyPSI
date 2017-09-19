#include "OPPRFSender.h"

#include "Crypto/Commit.h"
#include "Common/Log.h"
#include "Common/Log1.h"
#include "Common/Timer.h"
#include "Base/naor-pinkas.h"
#include "TwoChooseOne/IknpOtExtReceiver.h"
#include "TwoChooseOne/IknpOtExtSender.h"
#include "Parameters.h"

//#define PRINT
namespace osuCrypto
{
	void  OPPRFSender::sendPlain(u64 IdxP, binSet& bins, const std::vector<Channel*>& chls)
	{

		//std::vector<std::thread>  thrds(chls.size());
		std::vector<std::thread>  thrds(1);

		gTimer.setTimePoint("online.send.spaw");


		/*std::vector<block> hashIdxBlk(bins.mSimpleBins.mNumHashes[0] + bins.mSimpleBins.mNumHashes[1]);

		for (u64 i = 0; i < hashIdxBlk.size(); ++i)
		{
		hashIdxBlk[i] = _mm_set1_epi64x(i);
		}*/
		u32 numHashes = bins.mSimpleBins.mNumHashes[0] + bins.mSimpleBins.mNumHashes[1];

		//TODO: using vector
		uPtr<Buff> sendMaskBuff1(new Buff), sendMaskBuff2(new Buff), sendMaskBuff3(new Buff), 
			sendMaskBuff4(new Buff), sendMaskBuff5(new Buff);
		sendMaskBuff1->resize(mN* bins.mMaskSize);
		sendMaskBuff2->resize(mN* bins.mMaskSize);
		sendMaskBuff3->resize(mN* bins.mMaskSize);
		sendMaskBuff4->resize(mN* bins.mMaskSize);
		sendMaskBuff5->resize(mN* bins.mMaskSize);
		
		std::vector<MatrixView<u8>> maskBFView(numHashes);
		for (u64 i = 0; i < numHashes; i++)
		{
			if (i == 0)
				maskBFView[i] = sendMaskBuff1->getMatrixView<u8>(bins.mMaskSize);
			else if (i == 1)
				maskBFView[i] = sendMaskBuff2->getMatrixView<u8>(bins.mMaskSize);
			else if (i == 2)
				maskBFView[i] = sendMaskBuff3->getMatrixView<u8>(bins.mMaskSize);
			else if (i == 3)
				maskBFView[i] = sendMaskBuff4->getMatrixView<u8>(bins.mMaskSize);
			else if (i == 4)
				maskBFView[i] = sendMaskBuff5->getMatrixView<u8>(bins.mMaskSize);

		}

		for (u64 tIdx = 0; tIdx < thrds.size(); ++tIdx)
		{
			auto seed = mPrng.get<block>();
			thrds[tIdx] = std::thread([&, tIdx, seed]() {

				PRNG prng(seed);

				if (tIdx == 0) gTimer.setTimePoint("online.send.thrdStart");


				auto& chl = *chls[tIdx];

				if (tIdx == 0) gTimer.setTimePoint("online.send.insert");
				//const u64 stepSize = 16;

				std::vector<block> ncoInput(bins.mNcoInputBlkSize);

#if 1
#pragma region compute Send Bark-OPRF				
				//####################
				//#######Sender role
				//####################
				auto& otSend = *mOtSends[tIdx];
				auto otCountSend = bins.mSimpleBins.mBins.size();

				auto binStart = tIdx       * otCountSend / thrds.size();
				auto binEnd = (tIdx + 1) * otCountSend / thrds.size();

				if (tIdx == 0) gTimer.setTimePoint("online.send.OT");

				for (u64 bIdx = binStart; bIdx < binEnd;)
				{
					u64 currentStepSize = std::min(stepSize, binEnd - bIdx);
					otSend.recvCorrection(chl, currentStepSize);

					for (u64 stepIdx = 0; stepIdx < currentStepSize; ++bIdx, ++stepIdx)
					{

						auto& bin = bins.mSimpleBins.mBins[bIdx];


						if (bin.mIdx.size() > 0)
						{
							bin.mValOPRF[IdxP].resize(bin.mIdx.size());
							//std::cout << "s-" << inputIdx << ", ";
							for (u64 i = 0; i < bin.mIdx.size(); ++i)
							{

								u64 inputIdx = bin.mIdx[i];

								for (u64 j = 0; j < bins.mNcoInputBlkSize; ++j)
								{
									ncoInput[j] = bins.mNcoInputBuff[j][inputIdx];
								}

								//    block sendMask;

								otSend.encode(
									bIdx, //each bin has 1 OT
									ncoInput,
									bins.mSimpleBins.mOprfs[IdxP][inputIdx][bin.hIdx[i]]);
								//put oprf by inputIdx
								//std::cout << "------" << bins.mSimpleBins.mOprfs[IdxP][inputIdx][bin.hIdx[i]] << "\n";
								memcpy(maskBFView[bin.hIdx[i]][inputIdx].data(), (u8*)&bins.mSimpleBins.mOprfs[IdxP][inputIdx][bin.hIdx[i]], bins.mMaskSize);


							}
						}
					}
				}


				if (tIdx == 0) gTimer.setTimePoint("online.send.otSend.finalOPRF");

				/*for (size_t i = 0; i < 5; i++)
				{
					std::cout << "\nr[" << IdxP << "]-maskBFView.size() " << maskBFView[0].size()[0] << "\n";

				}*/

				chl.asyncSend(std::move(sendMaskBuff1));
				chl.asyncSend(std::move(sendMaskBuff2));
				chl.asyncSend(std::move(sendMaskBuff3));
				chl.asyncSend(std::move(sendMaskBuff4));
				chl.asyncSend(std::move(sendMaskBuff5));
#pragma endregion
#endif


				otSend.check(chl);

			});
		}

		for (auto& thrd : thrds)
			thrd.join();

		
	
		//std::cout << "\ns[" << IdxP << "]-maskBFView.size() " << maskBFView.size()[0] << "\n";
		//std::cout << "\ns[" << IdxP << "]-mBfSize " << mBfSize << "\n";
		//std::cout << "\ns[" << IdxP << "]-mMaskSize " << bins.mMaskSize << "\n";


		//std::cout << "s[" << IdxP << "]-dataSent(bytes)" << maskBFView.size()[0] * maskBFView.size()[1] << "----------\n";

		//std::cout << "\ns[" << IdxP << "]-arrayMask[1][3]" << arrayMask[1][3] << "\n";


	}


}



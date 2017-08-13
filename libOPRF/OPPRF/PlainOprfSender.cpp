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

	void OPPRFSender::sendPlain(u64 IdxP, binSet& bins,  const std::vector<Channel*>& chls)
	{
		u32 numHashes = bins.mSimpleBins.mNumHashes[0] + bins.mSimpleBins.mNumHashes[1];

		if (bins.mMaskSize > sizeof(block))
			throw std::runtime_error("masked are stored in blocks, so they can exceed that size");

		std::vector<std::thread>  thrds(chls.size());
		// std::vector<std::thread>  thrds(1);        


		uPtr<Buff> sendMaskBuff(new Buff);
		sendMaskBuff->resize(mN* bins.mMaskSize*numHashes);
		auto maskBFView = sendMaskBuff->getMatrixView<u8>(bins.mMaskSize);

		gTimer.setTimePoint("online.send.spaw");

		
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

						for (u64 hIdx = 0; hIdx < numHashes; ++hIdx)
						{
							bins.mSimpleBins.mOprfs[IdxP][inputIdx][hIdx];
							memcpy(maskBFView[hIdx*mN + inputIdx].data(), (u8*)&bins.mSimpleBins.mOprfs[IdxP][inputIdx][hIdx], bins.mMaskSize);

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

		//std::cout << "\ns[" << IdxP << "]-maskBFView.size() " << maskBFView.size()[0] << "\n";
		//std::cout << "\ns[" << IdxP << "]-mBfSize " << mBfSize << "\n";
		//std::cout << "\ns[" << IdxP << "]-mMaskSize " << bins.mMaskSize << "\n";


		std::cout << "s[" << IdxP << "]-dataSent(bytes)" << maskBFView.size()[0] * maskBFView.size()[1] << "----------\n";

		//std::cout << "\ns[" << IdxP << "]-arrayMask[1][3]" << arrayMask[1][3] << "\n";


		auto& chl = *chls[0];
		chl.asyncSend(std::move(sendMaskBuff));


	}


}



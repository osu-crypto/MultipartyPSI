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

	void OPPRFSender::init(u32 opt, u64 numParties, u64 setSize, u64 statSec, u64 inputBitSize,
		Channel & chl0, u64 otCounts,
		NcoOtExtSender&  otSend,
		NcoOtExtReceiver& otRecv,
		block seed, bool isOtherDirection)
	{
		init(opt, numParties, setSize, statSec, inputBitSize, { &chl0 }, otCounts, otSend, otRecv, seed, isOtherDirection);
	}


	void OPPRFSender::init(u32 opt, u64 numParties, u64 setSize, u64 statSec, u64 inputBitSize,
		const std::vector<Channel*>& chls, u64 otCounts,
		NcoOtExtSender& otSend,
		NcoOtExtReceiver& otRecv,
		block seed, bool isOtherDirection)
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

		if (opt == 3)
		{
			//######create hash
			mBFHasher.resize(mNumBFhashs);
			for (u64 i = 0; i < mBFHasher.size(); ++i)
				mBFHasher[i].setKey(_mm_set1_epi64x(i));// ^ mHashingSeed);

		}

		mPrng.SetSeed(seed);
		auto myHashSeed = mPrng.get<block>();
		auto& chl0 = *chls[0];


		/*	Commit comm(myHashSeed), theirComm;
		chl0.asyncSend(comm.data(), comm.size());
		chl0.recv(theirComm.data(), theirComm.size());


		chl0.asyncSend(&myHashSeed, sizeof(block));
		block theirHashingSeed;
		chl0.recv(&theirHashingSeed, sizeof(block));

		mHashingSeed = myHashSeed ^ theirHashingSeed;*/

		gTimer.setTimePoint("init.send.hashSeed");


		//	mSimpleBins.init(mN);
		//mCuckooBins.init(mN);

		//mPsis.resize(mBins.mBinCount);


		u64 otCountSend = otCounts;// mSimpleBins.mBins.size();
		u64 otCountRecv = otCounts; //mCuckooBins.mBins.size();


		gTimer.setTimePoint("init.send.baseStart");

		if (otSend.hasBaseOts() == false ||
			(otRecv.hasBaseOts() == false && isOtherDirection))
		{
			// first do 128 public key OTs (expensive)
			std::array<std::array<block, 2>, gOtExtBaseOtCount> baseMsg;
			NaorPinkas base;
			base.send(baseMsg, mPrng, chl0, 2);

			// now extend these to enough recv OTs to seed the send Kco and the send Kos ot extension
			u64 dualBaseOtCount = gOtExtBaseOtCount;

			if (!isOtherDirection) //if it is not dual, number extend OT is 128
				dualBaseOtCount = 0;

			BitVector recvChoice(baseOtCount + dualBaseOtCount); recvChoice.randomize(mPrng);
			std::vector<block> recvBaseMsg(baseOtCount + dualBaseOtCount);
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

			if (isOtherDirection) {
				// now lets extend these recv OTs in the other direction
				BitVector kosSendBaseChoice;
				kosSendBaseChoice.copy(recvChoice, baseOtCount, dualBaseOtCount);
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
		}

		gTimer.setTimePoint("init.send.extStart");


		auto sendRoutine = [&](u64 tIdx, u64 total, NcoOtExtSender& ots, Channel& chl)
		{
			auto start = (tIdx     * otCountSend / total);
			auto end = ((tIdx + 1) * otCountSend / total);

			ots.init(end - start);
		};


		auto recvOtRountine = [&](u64 tIdx, u64 total, NcoOtExtReceiver& ots, Channel& chl)
		{
			auto start = (tIdx     * otCountRecv / total);
			auto end = ((tIdx + 1) * otCountRecv / total);

			ots.init(end - start);
		};

		u64 numThreads = chls.size() - 1;
		u64 numSendThreads, numRecvThreads;

		if (isOtherDirection) {
			numSendThreads = numThreads / 2;
			numRecvThreads = numThreads - numSendThreads;
		}
		else
		{
			numSendThreads = numThreads;
			numRecvThreads = 0;
		}


		std::vector<std::thread> thrds(numThreads);
		auto thrdIter = thrds.begin();
		auto chlIter = chls.begin() + 1;

		mOtSends.resize(chls.size());

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
		mOtSends[0] = std::move(otSend.split());
		sendRoutine(0, numSendThreads + 1, *mOtSends[0], chl0);

		mOtRecvs.resize(chls.size());
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


		if (numRecvThreads == 0 && isOtherDirection)
		{
			mOtRecvs[0] = std::move(otRecv.split());

			recvOtRountine(0, 1, *mOtRecvs[0], chl0);
		}


		for (auto& thrd : thrds)
			thrd.join();

		gTimer.setTimePoint("init.send.done");

	}




	void OPPRFSender::getOPRFkeys(u64 IdxParty, binSet& bins, Channel & chl, bool isOtherDirectionGetOPRF)
	{

		if (bins.mOpt == TABLEb)
			getOPRFkeysSeperatedandTable(IdxParty, bins, { &chl }, isOtherDirectionGetOPRF);
		else if (bins.mOpt == SepPOLYb)
			getOPRFkeysSeperated(IdxParty, bins, { &chl }, isOtherDirectionGetOPRF);
		else
			getOPRFkeysCombined(IdxParty, bins, { &chl }, isOtherDirectionGetOPRF);
	}

	void OPPRFSender::getOPRFkeys(u64 IdxParty, binSet& bins, const std::vector<Channel*>& chls, bool isOtherDirectionGetOPRF)
	{
		if (bins.mOpt == TABLEb)
			getOPRFkeysSeperatedandTable(IdxParty, bins, chls, isOtherDirectionGetOPRF);
		else if (bins.mOpt == SepPOLYb)
			getOPRFkeysSeperated(IdxParty, bins, chls, isOtherDirectionGetOPRF);
		else
			getOPRFkeysCombined(IdxParty, bins, chls, isOtherDirectionGetOPRF);

	}

	void  OPPRFSender::getOPRFkeysSeperatedandTable(u64 IdxP, binSet& bins, const std::vector<Channel*>& chls, bool isOtherDirectionGetOPRF)
	{

		//std::vector<std::thread>  thrds(chls.size());
		std::vector<std::thread>  thrds(1);

		gTimer.setTimePoint("online.send.spaw");


		for (u64 tIdx = 0; tIdx < thrds.size(); ++tIdx)
		{
			auto seed = mPrng.get<block>();
			thrds[tIdx] = std::thread([&, tIdx, seed]() {

				PRNG prng(seed);

				if (tIdx == 0) gTimer.setTimePoint("online.send.thrdStart");


				auto& chl = *chls[tIdx];

				if (tIdx == 0) gTimer.setTimePoint("online.send.insert");
				const u64 stepSize = 16;

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
							//std::cout << "s-" << bIdx << ", ";
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
									bin.mValOPRF[IdxP][i]);
								//mmOPRF[bIdx][i]);
								/*if (bIdx < 3 || (bIdx < mN && bIdx > mN - 2))
								std::cout << "s-"<<bIdx <<", "<< inputIdx << ": " << sendMask << std::endl;*/
							}

							//#####################
							//######Finding bit locations
							//#####################

							//std::cout << bin.mValOPRF[IdxP][0];

							//diff max bin size for first mSimpleBins.mBinCount and 
							// mSimpleBins.mBinStashCount
							if (bIdx < bins.mSimpleBins.mBinCount[0])
								bin.mBits[IdxP].init(/*bin.mIdx.size(), */bins.mSimpleBins.mNumBits[0]);
							else
								bin.mBits[IdxP].init(/*bin.mIdx.size(), */bins.mSimpleBins.mNumBits[1]);

							auto start = mTimer.setTimePoint("getPos1.start");
							bin.mBits[IdxP].getPos1(bin.mValOPRF[IdxP], 128);
							auto end = mTimer.setTimePoint("getPos1.done");

							mPosBitsTime += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

							//bin.mBits[IdxP].getMasks(bin.mValOPRF[IdxP]);
							//std::cout << ", "
							//	<< static_cast<int16_t>(bin.mBits[IdxP].mMaps[0]) << std::endl;
						}
					}
				}


				if (tIdx == 0) gTimer.setTimePoint("online.send.otSend.finalOPRF");

#ifdef PRINT
				std::cout << "getPosTime" << IdxP << ": " << mPosBitsTime / pow(10, 6) << std::endl;
#endif // PRINT


#pragma endregion
#endif

#if 1
#pragma region compute Recv Bark-OPRF

				//####################
				//#######Receiver role
				//####################
				if (isOtherDirectionGetOPRF) {
					auto& otRecv = *mOtRecvs[tIdx];
					auto otCountRecv = bins.mCuckooBins.mBins.size();
					// get the region of the base OTs that this thread should do.
					binStart = tIdx       * otCountRecv / thrds.size();
					binEnd = (tIdx + 1) * otCountRecv / thrds.size();

					for (u64 bIdx = binStart; bIdx < binEnd;)
					{
						u64 currentStepSize = std::min(stepSize, binEnd - bIdx);

						for (u64 stepIdx = 0; stepIdx < currentStepSize; ++bIdx, ++stepIdx)
						{

							auto& bin = bins.mCuckooBins.mBins[bIdx];

							if (!bin.isEmpty())
							{
								u64 inputIdx = bin.idx();

								for (u64 j = 0; j < ncoInput.size(); ++j)
									ncoInput[j] = bins.mNcoInputBuff[j][inputIdx];

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

					otRecv.check(chl);
				}
#pragma endregion
#endif
				otSend.check(chl);

			});
		}

		for (auto& thrd : thrds)
			thrd.join();
	}


	void  OPPRFSender::getOPRFkeysSeperated(u64 IdxP, binSet& bins, const std::vector<Channel*>& chls, bool isOtherDirectionGetOPRF)
	{

		//std::vector<std::thread>  thrds(chls.size());
		std::vector<std::thread>  thrds(1);

		gTimer.setTimePoint("online.send.spaw");


		for (u64 tIdx = 0; tIdx < thrds.size(); ++tIdx)
		{
			auto seed = mPrng.get<block>();
			thrds[tIdx] = std::thread([&, tIdx, seed]() {

				PRNG prng(seed);

				if (tIdx == 0) gTimer.setTimePoint("online.send.thrdStart");


				auto& chl = *chls[tIdx];

				if (tIdx == 0) gTimer.setTimePoint("online.send.insert");
				const u64 stepSize = 16;

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
									bin.mValOPRF[IdxP][i]);
								//mmOPRF[inputIdx][i]);
								/*if (inputIdx < 3 || (inputIdx < mN && inputIdx > mN - 2))
								std::cout << "s-"<<inputIdx <<", "<< inputIdx << ": " << sendMask << std::endl;*/
							}
						}
					}
				}


				if (tIdx == 0) gTimer.setTimePoint("online.send.otSend.finalOPRF");

#ifdef PRINT
				std::cout << "getPosTime" << IdxP << ": " << mPosBitsTime / pow(10, 6) << std::endl;
#endif // PRINT


#pragma endregion
#endif

#if 1
#pragma region compute Recv Bark-OPRF

				//####################
				//#######Receiver role
				//####################
				if (isOtherDirectionGetOPRF) {
					auto& otRecv = *mOtRecvs[tIdx];
					auto otCountRecv = bins.mCuckooBins.mBins.size();
					// get the region of the base OTs that this thread should do.
					binStart = tIdx       * otCountRecv / thrds.size();
					binEnd = (tIdx + 1) * otCountRecv / thrds.size();

					for (u64 bIdx = binStart; bIdx < binEnd;)
					{
						u64 currentStepSize = std::min(stepSize, binEnd - bIdx);

						for (u64 stepIdx = 0; stepIdx < currentStepSize; ++bIdx, ++stepIdx)
						{

							auto& bin = bins.mCuckooBins.mBins[bIdx];

							if (!bin.isEmpty())
							{
								u64 inputIdx = bin.idx();

								for (u64 j = 0; j < ncoInput.size(); ++j)
									ncoInput[j] = bins.mNcoInputBuff[j][inputIdx];

								otRecv.encode(
									bIdx,      // input
									ncoInput,             // input
									bin.mValOPRF[IdxP]); // output

														 /*if (inputIdx < 3 || (inputIdx < mN && inputIdx > mN-2))
														 std::cout << "r-" << inputIdx << ", " << inputIdx << ": " << valOPRF[inputIdx] << std::endl;*/
							}
							else
							{
								otRecv.zeroEncode(bIdx);
							}
						}
						otRecv.sendCorrection(chl, currentStepSize);
					}

					if (tIdx == 0) gTimer.setTimePoint("online.send.otRecv.finalOPRF");

					otRecv.check(chl);
				}
#pragma endregion
#endif
				otSend.check(chl);

			});
		}

		for (auto& thrd : thrds)
			thrd.join();
	}


	void  OPPRFSender::getOPRFkeysCombined(u64 IdxP, binSet& bins, const std::vector<Channel*>& chls, bool isOtherDirectionGetOPRF)
	{

		//std::vector<std::thread>  thrds(chls.size());
		std::vector<std::thread>  thrds(1);

		gTimer.setTimePoint("online.send.spaw");


		/*std::vector<block> hashIdxBlk(bins.mSimpleBins.mNumHashes[0] + bins.mSimpleBins.mNumHashes[1]);

		for (u64 i = 0; i < hashIdxBlk.size(); ++i)
		{
		hashIdxBlk[i] = _mm_set1_epi64x(i);
		}*/

		for (u64 tIdx = 0; tIdx < thrds.size(); ++tIdx)
		{
			auto seed = mPrng.get<block>();
			thrds[tIdx] = std::thread([&, tIdx, seed]() {

				PRNG prng(seed);

				if (tIdx == 0) gTimer.setTimePoint("online.send.thrdStart");


				auto& chl = *chls[tIdx];

				if (tIdx == 0) gTimer.setTimePoint("online.send.insert");
				const u64 stepSize = 16;

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


							}
						}
					}
				}


				if (tIdx == 0) gTimer.setTimePoint("online.send.otSend.finalOPRF");

#pragma endregion
#endif

#if 1
#pragma region compute Recv Bark-OPRF

				//####################
				//#######Receiver role
				//####################
				if (isOtherDirectionGetOPRF) {
					auto& otRecv = *mOtRecvs[tIdx];
					auto otCountRecv = bins.mCuckooBins.mBins.size();
					// get the region of the base OTs that this thread should do.
					binStart = tIdx       * otCountRecv / thrds.size();
					binEnd = (tIdx + 1) * otCountRecv / thrds.size();

					for (u64 bIdx = binStart; bIdx < binEnd;)
					{
						u64 currentStepSize = std::min(stepSize, binEnd - bIdx);

						for (u64 stepIdx = 0; stepIdx < currentStepSize; ++bIdx, ++stepIdx)
						{

							auto& bin = bins.mCuckooBins.mBins[bIdx];

							if (!bin.isEmpty())
							{
								u64 inputIdx = bin.idx();

								for (u64 j = 0; j < ncoInput.size(); ++j)
									ncoInput[j] = bins.mNcoInputBuff[j][inputIdx];

								otRecv.encode(
									bIdx,      // input
									ncoInput,             // input
									bin.mValOPRF[IdxP]); // output

														 /*if (inputIdx < 3 || (inputIdx < mN && inputIdx > mN-2))
														 std::cout << "r-" << inputIdx << ", " << inputIdx << ": " << valOPRF[inputIdx] << std::endl;*/
							}
							else
							{
								otRecv.zeroEncode(bIdx);
							}
						}
						otRecv.sendCorrection(chl, currentStepSize);
					}

					if (tIdx == 0) gTimer.setTimePoint("online.send.otRecv.finalOPRF");

					otRecv.check(chl);
				}
#pragma endregion
#endif
				otSend.check(chl);

			});
		}

		for (auto& thrd : thrds)
			thrd.join();
	}



	void OPPRFSender::sendSS(u64 IdxParty, binSet& bins, std::vector<block>& plaintexts, Channel & chl)
	{
		if (bins.mOpt == TABLEb)
			sendSSTableBased(IdxParty, bins, plaintexts, { &chl });
		else if (bins.mOpt == SepPOLYb)
			sendSSPolyBased(IdxParty, bins, plaintexts, { &chl });
		else if (bins.mOpt == OnePOLYb)
			sendFullPolyBased(IdxParty, bins, plaintexts, { &chl });
		else if (bins.mOpt == BFb)
			sendBFBased(IdxParty, bins, plaintexts, { &chl });
	}

	void OPPRFSender::recvSS(u64 IdxParty, binSet& bins, std::vector<block>& plaintexts, Channel & chl)
	{
		if (bins.mOpt == TABLEb)
			recvSSTableBased(IdxParty, bins, plaintexts, { &chl });
		else if (bins.mOpt == SepPOLYb)
			recvSSPolyBased(IdxParty, bins, plaintexts, { &chl });
		else if (bins.mOpt == OnePOLYb)
			recvFullPolyBased(IdxParty, bins, plaintexts, { &chl });
		else if (bins.mOpt == BFb)
			recvBFBased(IdxParty, bins, plaintexts, { &chl });
	}

	void OPPRFSender::sendSS(u64 IdxParty, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls)
	{
		if (bins.mOpt == TABLEb)
			sendSSTableBased(IdxParty, bins, plaintexts, chls);
		else if (bins.mOpt == SepPOLYb)
			sendSSPolyBased(IdxParty, bins, plaintexts, chls);
		else if (bins.mOpt == OnePOLYb)
			sendFullPolyBased(IdxParty, bins, plaintexts, chls);
		else if (bins.mOpt == BFb)
			sendBFBased(IdxParty, bins, plaintexts, chls);
	}

	void OPPRFSender::recvSS(u64 IdxParty, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls)
	{
		if (bins.mOpt == TABLEb)
			recvSSTableBased(IdxParty, bins, plaintexts, chls);
		else if (bins.mOpt == SepPOLYb)
			recvSSPolyBased(IdxParty, bins, plaintexts, chls); // gf2x is not thread safe =>USING 1 thread
		else if (bins.mOpt == OnePOLYb)
			recvFullPolyBased(IdxParty, bins, plaintexts, chls);// gf2x is not thread safe =>USING 1 thread
		else if (bins.mOpt == BFb)
			recvBFBased(IdxParty, bins, plaintexts, chls);
	}


}



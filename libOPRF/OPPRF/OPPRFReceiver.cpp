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
	OPPRFReceiver::OPPRFReceiver()
	{
	}

	OPPRFReceiver::~OPPRFReceiver()
	{
	}

	void OPPRFReceiver::init(u32 opt, u64 numParties,
		u64 n,
		u64 statSec,
		u64 inputBitSize,
		Channel & chl0, u64 otCounts,
		NcoOtExtReceiver& otRecv,
		NcoOtExtSender& otSend,
		block seed, bool isOtherDirection)
	{
		init(opt, numParties, n, statSec, inputBitSize, { &chl0 }, otCounts, otRecv, otSend, seed, isOtherDirection);
	}


	void OPPRFReceiver::init(u32 opt, u64 numParties,
		u64 n,
		u64 statSecParam,
		u64 inputBitSize,
		const std::vector<Channel*>& chls, u64 otCounts,
		NcoOtExtReceiver& otRecv,
		NcoOtExtSender& otSend,
		block seed, bool isOtherDirection)
	{

		//testReceiver();
		// this is the offline function for doing binning and then performing the OtPsi* between the bins.
		mParties = numParties;
		mStatSecParam = statSecParam;
		mN = n;



		// must be a multiple of 128...
		u64 baseOtCount;// = 128 * CodeWordSize;
		u64 compSecParam = 128;

		otSend.getParams(
			false,
			compSecParam, statSecParam, inputBitSize, mN, //  input
			mNcoInputBlkSize, baseOtCount); // output

											//mOtMsgBlkSize = (baseOtCount + 127) / 128;
		if (opt == 3)
		{
			//######create hash
			mBFHasher.resize(mNumBFhashs);
			for (u64 i = 0; i < mBFHasher.size(); ++i)
				mBFHasher[i].setKey(_mm_set1_epi64x(i));// ^ mHashingSeed);
		}


		gTimer.setTimePoint("Init.recv.start");
		mPrng.SetSeed(seed);
		auto& prng = mPrng;

		auto myHashSeed = prng.get<block>();

		auto& chl0 = *chls[0];

		// we need a random hash function, so we will both commit to a seed and then later decommit. 
		//This is the commitments phase
		//Commit comm(myHashSeed), theirComm;
		//chl0.asyncSend(comm.data(), comm.size());
		//chl0.recv(theirComm.data(), theirComm.size());

		//// ok, now decommit to the seed.
		//chl0.asyncSend(&myHashSeed, sizeof(block));
		//block theirHashingSeed;
		//chl0.recv(&theirHashingSeed, sizeof(block));

		//gTimer.setTimePoint("Init.recv.hashSeed");

		//// compute the hashing seed as the xor of both of ours seeds.
		//mHashingSeed = myHashSeed ^ theirHashingSeed;


		// how many OTs we need in total.
		u64 otCountSend = otCounts;// mSimpleBins.mBins.size();
		u64 otCountRecv = otCounts; //mCuckooBins.mBins.size();


		gTimer.setTimePoint("Init.recv.baseStart");
		// since we are doing mmlicious PSI, we need OTs going in both directions. 
		// This will hold the send OTs

		if (otRecv.hasBaseOts() == false ||
			(otSend.hasBaseOts() == false && isOtherDirection))
		{
			// first do 128 public key OTs (expensive)
			std::array<block, gOtExtBaseOtCount> kosSendBase;
			BitVector choices(gOtExtBaseOtCount); choices.randomize(prng);
			NaorPinkas base;
			base.receive(choices, kosSendBase, prng, chl0, 2);


			// now extend these to enough recv OTs to seed the send Kco and the send Kos ot extension
			u64 dualBaseOtCount = gOtExtBaseOtCount;
			if (!isOtherDirection) //if it is not dual, number extend OT is 128
				dualBaseOtCount = 0;

			IknpOtExtSender iknpSend;
			iknpSend.setBaseOts(kosSendBase, choices);
			std::vector<std::array<block, 2>> sendBaseMsg(baseOtCount + dualBaseOtCount);
			iknpSend.send(sendBaseMsg, prng, chl0);


			// Divide these OT mssages between the Kco and Kos protocols
			ArrayView<std::array<block, 2>> kcoRecvBase(
				sendBaseMsg.begin(),
				sendBaseMsg.begin() + baseOtCount);
			// now set these ~800 OTs as the base of our N choose 1 OTs.
			otRecv.setBaseOts(kcoRecvBase);

			if (isOtherDirection) {
				ArrayView<std::array<block, 2>> kosRecvBase(
					sendBaseMsg.begin() + baseOtCount,
					sendBaseMsg.end());

				BitVector recvChoice(baseOtCount); recvChoice.randomize(prng);
				std::vector<block> kcoSendBase(baseOtCount);
				IknpOtExtReceiver iknp;
				iknp.setBaseOts(kosRecvBase);
				iknp.receive(recvChoice, kcoSendBase, prng, chl0);
				// now set these ~800 OTs as the base of our N choose 1 OTs.
				otSend.setBaseOts(kcoSendBase, recvChoice);
			}

		}


		gTimer.setTimePoint("Init.recv.ExtStart");




		auto sendOtRoutine = [&](u64 tIdx, u64 total, NcoOtExtSender& ots, Channel& chl)
		{
			auto start = (tIdx     *otCountSend / total);
			auto end = ((tIdx + 1) * otCountSend / total);

			ots.init(end - start);
		};

		auto recvOtRoutine = [&](u64 tIdx, u64 total, NcoOtExtReceiver& ots, Channel& chl)
		{
			auto start = (tIdx     * otCountRecv / total);
			auto end = ((tIdx + 1) * otCountRecv / total);

			ots.init(end - start);
		};


		// compute how amny threads we want to do for each direction.
		// the current thread will do one of the OT receives so -1 for that.
		u64 numThreads = chls.size() - 1;
		u64 numRecvThreads, numSendThreads;

		if (isOtherDirection) {
			numRecvThreads = numThreads / 2;
			numSendThreads = numThreads - numRecvThreads;
		}
		else {
			numRecvThreads = numThreads;
			numSendThreads = 0;
		}
		// where we will store the threads that are doing the extension
		std::vector<std::thread> thrds(numThreads);

		// some iters to help giving out resources.
		auto thrdIter = thrds.begin();
		auto chlIter = chls.begin() + 1;

		mOtRecvs.resize(chls.size());

		// now make the threads that will to the extension
		for (u64 i = 0; i < numRecvThreads; ++i)
		{
			mOtRecvs[i + 1] = std::move(otRecv.split());

			// spawn the thread and call the routine.
			*thrdIter++ = std::thread([&, i, chlIter]()
			{
				recvOtRoutine(i + 1, numRecvThreads + 1, *mOtRecvs[i + 1], **chlIter);
			});

			++chlIter;
		}
		mOtRecvs[0] = std::move(otRecv.split());
		// now use this thread to do a recv routine.
		recvOtRoutine(0, numRecvThreads + 1, *mOtRecvs[0], chl0);



		mOtSends.resize(chls.size());
		// do the same thing but for the send OT extensions
		for (u64 i = 0; i < numSendThreads; ++i)
		{

			mOtSends[i] = std::move(otSend.split());

			*thrdIter++ = std::thread([&, i, chlIter]()
			{
				sendOtRoutine(i, numSendThreads, *mOtSends[i], **chlIter);
			});

			++chlIter;
		}

		// if the caller doesnt want to do things in parallel
		// the we will need to do the send OT Ext now...
		if (numSendThreads == 0 && isOtherDirection)
		{
			mOtSends[0] = std::move(otSend.split());
			sendOtRoutine(0, 1, *mOtSends[0], chl0);
		}

		// join any threads that we created.
		for (auto& thrd : thrds)
			thrd.join();

		gTimer.setTimePoint("Init.recv.done");

	}



	void OPPRFReceiver::getOPRFkeys(u64 IdxParty, binSet& bins, Channel & chl, bool isOtherDirectionGetOPRF)
	{

		if (bins.mOpt == TABLEb)
			getOPRFkeysSeperatedandTable(IdxParty, bins, { &chl }, isOtherDirectionGetOPRF);
		else if (bins.mOpt == SepPOLYb)
			getOPRFkeysSeperated(IdxParty, bins, { &chl }, isOtherDirectionGetOPRF);
		else
			getOPRFkeysCombined(IdxParty, bins, { &chl }, isOtherDirectionGetOPRF);
	}

	void OPPRFReceiver::getOPRFkeys(u64 IdxParty, binSet& bins, const std::vector<Channel*>& chls, bool isOtherDirectionGetOPRF)
	{
		if (bins.mOpt == TABLEb)
			getOPRFkeysSeperatedandTable(IdxParty, bins, chls, isOtherDirectionGetOPRF);
		else if (bins.mOpt == SepPOLYb)
			getOPRFkeysSeperated(IdxParty, bins, chls, isOtherDirectionGetOPRF);
		else
			getOPRFkeysCombined(IdxParty, bins, chls, isOtherDirectionGetOPRF);

	}

	void OPPRFReceiver::sendSS(u64 IdxParty, binSet& bins, std::vector<block>& plaintexts, Channel & chl)
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
	void OPPRFReceiver::recvSS(u64 IdxParty, binSet& bins, std::vector<block>& plaintexts, Channel & chl)
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

	void OPPRFReceiver::sendSS(u64 IdxParty, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls)
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

	void OPPRFReceiver::recvSS(u64 IdxParty, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls)
	{
		if (bins.mOpt == TABLEb)
			recvSSTableBased(IdxParty, bins, plaintexts, chls);
		else if (bins.mOpt == SepPOLYb)
			recvSSPolyBased(IdxParty, bins, plaintexts, chls);
		else if (bins.mOpt == OnePOLYb)
			recvFullPolyBased(IdxParty, bins, plaintexts, chls);
		else if (bins.mOpt == BFb)
			recvBFBased(IdxParty, bins, plaintexts, chls);
	}

	void OPPRFReceiver::getOPRFkeysSeperatedandTable(u64 IdxP, binSet& bins, const std::vector<Channel*>& chls, bool isOtherDirectionGetOPRF)
	{
#if 1
		// this is the online phase.
		gTimer.setTimePoint("online.recv.start");


		std::vector<std::thread>  thrds(chls.size());
		//  std::vector<std::thread>  thrds(1);

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

							for (u64 j = 0; j < ncoInput.size(); ++j)
								ncoInput[j] = bins.mNcoInputBuff[j][inputIdx];

							otRecv.encode(
								bIdx,      // input
								ncoInput,             // input
								bin.mValOPRF[IdxP]); // output
						}
						else
							otRecv.zeroEncode(bIdx);
					}
					otRecv.sendCorrection(chl, currentStepSize);
				}

				if (tIdx == 0) gTimer.setTimePoint("online.recv.otRecv.finalOPRF");



#pragma endregion
#endif

#if 1
#pragma region compute Send Bark-OPRF				
				//####################
				//#######Sender role
				//####################
				if (isOtherDirectionGetOPRF) {
					auto& otSend = *mOtSends[tIdx];
					auto otCountSend = bins.mSimpleBins.mBins.size();

					binStart = tIdx       * otCountSend / thrds.size();
					binEnd = (tIdx + 1) * otCountSend / thrds.size();


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

									for (u64 j = 0; j < mNcoInputBlkSize; ++j)
									{
										ncoInput[j] = bins.mNcoInputBuff[j][inputIdx];
									}

									otSend.encode(
										bIdx, //each bin has 1 OT
										ncoInput,
										bin.mValOPRF[IdxP][i]);

								}

								//#####################
								//######Finding bit locations
								//#####################

								//	std::cout << bin.mValOPRF[IdxP][0];

								//diff max bin size for first mSimpleBins.mBinCount and 
								// mSimpleBins.mBinStashCount
								if (bIdx < bins.mSimpleBins.mBinCount[0])
									bin.mBits[IdxP].init(/*bin.mIdx.size(),*/ bins.mSimpleBins.mNumBits[0]);
								else
									bin.mBits[IdxP].init(/*bin.mIdx.size(),*/ bins.mSimpleBins.mNumBits[1]);

								bin.mBits[IdxP].getPos1(bin.mValOPRF[IdxP], 128);
								//bin.mBits[IdxP].getMasks(bin.mValOPRF[IdxP]);
								//std::cout << ", "
								//	<< static_cast<int16_t>(bin.mBits[IdxP].mMaps[0]) << std::endl;
							}
						}
					}
					if (tIdx == 0) gTimer.setTimePoint("online.send.otSend.finalOPRF");
					otSend.check(chl);
				}
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
	}

	void  OPPRFReceiver::getOPRFkeysSeperated(u64 IdxP, binSet& bins, const std::vector<Channel*>& chls, bool isOtherDirectionGetOPRF)
	{
#if 1
		// this is the online phase.
		gTimer.setTimePoint("online.recv.start");


		std::vector<std::thread>  thrds(chls.size());
		//  std::vector<std::thread>  thrds(1);

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

							for (u64 j = 0; j < ncoInput.size(); ++j)
								ncoInput[j] = bins.mNcoInputBuff[j][inputIdx];

							otRecv.encode(
								bIdx,      // input
								ncoInput,             // input
								bin.mValOPRF[IdxP]); // output
						}
						else
							otRecv.zeroEncode(bIdx);
					}
					otRecv.sendCorrection(chl, currentStepSize);
				}

				if (tIdx == 0) gTimer.setTimePoint("online.recv.otRecv.finalOPRF");



#pragma endregion
#endif

#if 1
#pragma region compute Send Bark-OPRF				
				//####################
				//#######Sender role
				//####################
				if (isOtherDirectionGetOPRF) {
					auto& otSend = *mOtSends[tIdx];
					auto otCountSend = bins.mSimpleBins.mBins.size();

					binStart = tIdx       * otCountSend / thrds.size();
					binEnd = (tIdx + 1) * otCountSend / thrds.size();


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

									for (u64 j = 0; j < mNcoInputBlkSize; ++j)
									{
										ncoInput[j] = bins.mNcoInputBuff[j][inputIdx];
									}

									otSend.encode(
										bIdx, //each bin has 1 OT
										ncoInput,
										bin.mValOPRF[IdxP][i]);

								}
							}
						}
					}
					if (tIdx == 0) gTimer.setTimePoint("online.send.otSend.finalOPRF");
					otSend.check(chl);
				}
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
	}

	void  OPPRFReceiver::getOPRFkeysCombined(u64 IdxP, binSet& bins, const std::vector<Channel*>& chls, bool isOtherDirectionGetOPRF)
	{
#if 1
		// this is the online phase.
		gTimer.setTimePoint("online.recv.start");


		std::vector<std::thread>  thrds(chls.size());
		//  std::vector<std::thread>  thrds(1);

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

							for (u64 j = 0; j < ncoInput.size(); ++j)
								ncoInput[j] = bins.mNcoInputBuff[j][inputIdx];

							otRecv.encode(
								bIdx,      // input
								ncoInput,             // input
								bin.mValOPRF[IdxP]); // output
						}
						else
							otRecv.zeroEncode(bIdx);
					}
					otRecv.sendCorrection(chl, currentStepSize);
				}

				if (tIdx == 0) gTimer.setTimePoint("online.recv.otRecv.finalOPRF");



#pragma endregion
#endif

#if 1
#pragma region compute Send Bark-OPRF				
				//####################
				//#######Sender role
				//####################
				if (isOtherDirectionGetOPRF) {
					auto& otSend = *mOtSends[tIdx];
					auto otCountSend = bins.mSimpleBins.mBins.size();

					binStart = tIdx       * otCountSend / thrds.size();
					binEnd = (tIdx + 1) * otCountSend / thrds.size();


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

									for (u64 j = 0; j < mNcoInputBlkSize; ++j)
									{
										ncoInput[j] = bins.mNcoInputBuff[j][inputIdx];
									}

									otSend.encode(
										bIdx, //each bin has 1 OT
										ncoInput,
										bins.mSimpleBins.mOprfs[IdxP][inputIdx][bin.hIdx[i]]);//put oprf by inputIdx
																							  //bin.mValOPRF[IdxP][i]);

								}
							}
						}
					}
					if (tIdx == 0) gTimer.setTimePoint("online.send.otSend.finalOPRF");
					otSend.check(chl);
				}
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
	}

}
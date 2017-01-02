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
#include "Hashing/BitPosition.h"
//#define PRINT
namespace osuCrypto
{
    OPPRFReceiver::OPPRFReceiver()
    {
    }

    OPPRFReceiver::~OPPRFReceiver()
    {
    }

    void OPPRFReceiver::init(u64 numParties,
        u64 n,
        u64 statSec,
        u64 inputBitSize,
        Channel & chl0, u64 otCounts,
		NcoOtExtReceiver& otRecv,
		NcoOtExtSender& otSend,
        block seed, bool isOtherDirection)
    {
        init(numParties,n, statSec, inputBitSize, { &chl0 }, otCounts, otRecv, otSend, seed, isOtherDirection);
    }

    void OPPRFReceiver::init(u64 numParties,
			u64 n,
			u64 statSecParam,
			u64 inputBitSize,
			const std::vector<Channel*>& chls, u64 otCounts,
			NcoOtExtReceiver& otRecv,
			NcoOtExtSender& otSend,
			block seed, bool isOtherDirection)
	{

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


		gTimer.setTimePoint("Init.recv.start");
		mPrng.SetSeed(seed);
		auto& prng = mPrng;

		auto myHashSeed = prng.get<block>();

		auto& chl0 = *chls[0];

		// we need a random hash function, so we will both commit to a seed and then later decommit. 
		//This is the commitments phase
		Commit comm(myHashSeed), theirComm;
		chl0.asyncSend(comm.data(), comm.size());
		chl0.recv(theirComm.data(), theirComm.size());

		// ok, now decommit to the seed.
		chl0.asyncSend(&myHashSeed, sizeof(block));
		block theirHashingSeed;
		chl0.recv(&theirHashingSeed, sizeof(block));

		gTimer.setTimePoint("Init.recv.hashSeed");

		// compute the hashing seed as the xor of both of ours seeds.
		mHashingSeed = myHashSeed ^ theirHashingSeed;


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
			auto start = (tIdx     *otCountSend / total) ;
			auto end = ((tIdx + 1) * otCountSend / total);

			ots.init(end - start);
		};

		auto recvOtRoutine = [&](u64 tIdx, u64 total, NcoOtExtReceiver& ots, Channel& chl)
		{
			auto start = (tIdx     * otCountRecv / total) ;
			auto end = ((tIdx + 1) * otCountRecv / total) ;

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
		getOPRFkeys( IdxParty, bins,{ &chl }, isOtherDirectionGetOPRF);
	}

	void OPPRFReceiver::getOPRFkeys(u64 IdxP, binSet& bins, const std::vector<Channel*>& chls, bool isOtherDirectionGetOPRF)
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




	void OPPRFReceiver::revSecretSharing(u64 IdxParty, binSet& bins, std::vector<block>& plaintexts, Channel & chl)
	{
		revSecretSharing(IdxParty, bins,plaintexts, { &chl });
	}

	void OPPRFReceiver::revSecretSharing(u64 IdxP, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls)
	{

		// this is the online phase.
		gTimer.setTimePoint("online.recv.start");

		//u64 maskSize = sizeof(block);// roundUpTo(mStatSecParam + 2 * std::log(mN) - 1, 8) / 8;
		u64 maskSize = roundUpTo(mStatSecParam + 2 * std::log(mN) - 1, 8) / 8;
		if (maskSize > sizeof(block))
			throw std::runtime_error("masked are stored in blocks, so they can exceed that size");


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

						

					//use the params of the simple hashing as their params
					u64 mTheirBins_mMaxBinSize = bins.mSimpleBins.mMaxBinSize[bIdxType];
					u64 mTheirBins_mNumBits= bins.mSimpleBins.mNumBits[bIdxType];
					for (u64 bIdx = binStart; bIdx < binEnd;)
					{
						u64 curStepSize = std::min(stepSize, binEnd - bIdx);

						MatrixView<u8> maskView;
						ByteStream maskBuffer;
						chl.recv(maskBuffer);
						//maskView = maskBuffer.getMatrixView<u8>(mTheirBins_mMaxBinSize * maskSize + mTheirBins_mNumBits * sizeof(u8));
						maskView = maskBuffer.getMatrixView<u8>(mTheirBins_mMaxBinSize * maskSize + mTheirBins_mNumBits * sizeof(u8));
						if (maskView.size()[0] != curStepSize)
							throw std::runtime_error("size not expedted");

						for (u64 stepIdx = 0; stepIdx < curStepSize; ++bIdx, ++stepIdx)
						{

							auto& bin = bins.mCuckooBins.mBins[bIdx];
							if (!bin.isEmpty())
							{
								u64 baseMaskIdx = stepIdx;
								auto mask = maskView[baseMaskIdx];
								BitPosition b;
								b.mMaxBitSize = mTheirBins_mNumBits;
								for (u64 i = 0; i < b.mMaxBitSize; i++)
								{
									int idxPos = 0;
									memcpy(&idxPos, maskView[baseMaskIdx].data() + i, sizeof(u8));
									b.mPos.push_back(idxPos);
								}
#ifdef PRINT
								Log::out << "RBin #" << bIdx << Log::endl;
								Log::out << "    cc_mPos= ";
								for (u64 idxPos = 0; idxPos < b.mPos.size(); idxPos++)
								{
									Log::out << static_cast<int16_t>(b.mPos[idxPos]) << " ";
								}
								Log::out << Log::endl;
#endif
								u64 inputIdx = bin.idx();
								auto myMask = bin.mValOPRF[IdxP];
								//	u8 myMaskPos = 0;
								b.getMask(myMask, bin.mValMap[IdxP]);

								u64	MaskIdx = bin.mValMap[IdxP]*maskSize + mTheirBins_mNumBits;

								auto theirMask = ZeroBlock;
								memcpy(&theirMask, maskView[baseMaskIdx].data() + MaskIdx, maskSize);

								//if (!memcmp((u8*)&myMask, &theirMask, maskSize))
								//{
								//Log::out << "inputIdx: " << inputIdx << Log::endl;
								//	Log::out << "myMask: " << myMask << Log::endl;
								//Log::out << "theirMask: " << theirMask << " " << Log::endl;
			

								plaintexts[inputIdx] = myMask^theirMask;


								//}
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
		
		// check that the number of inputs is as expected.
		if (plaintexts.size() != mN)
			throw std::runtime_error(LOCATION);

		

	}


	void OPPRFReceiver::sendSecretSharing(u64 IdxParty, binSet& bins, std::vector<block>& plaintexts, Channel & chl)
	{
		sendSecretSharing(IdxParty, bins, plaintexts, { &chl });
	}

	void OPPRFReceiver::sendSecretSharing(u64 IdxP, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls)
	{
		if (plaintexts.size() != mN)
			throw std::runtime_error(LOCATION);


		//TODO: double check
		u64 maskSize = roundUpTo(mStatSecParam + 2 * std::log(mN) - 1, 8) / 8;
									 //u64 maskSize = 7;
		if (maskSize > sizeof(block))
			throw std::runtime_error("masked are stored in blocks, so they can exceed that size");



		std::vector<std::thread>  thrds(chls.size());
		// std::vector<std::thread>  thrds(1);        

		std::mutex mtx;


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

				//2 type of bins: normal bin in inital step + stash bin
				for (auto bIdxType = 0; bIdxType < 2; bIdxType++)
				{
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
						sendMaskBuff->resize(currentStepSize * (bins.mSimpleBins.mMaxBinSize[bIdxType] * maskSize + bins.mSimpleBins.mNumBits[bIdxType] * sizeof(u8)));
						auto maskView = sendMaskBuff->getMatrixView<u8>(bins.mSimpleBins.mMaxBinSize[bIdxType] * maskSize + bins.mSimpleBins.mNumBits[bIdxType] * sizeof(u8));

						for (u64 stepIdx = 0; stepIdx < currentStepSize; ++bIdx, ++stepIdx)
						{
							//Log::out << "sBin #" << bIdx << Log::endl;

							auto& bin = bins.mSimpleBins.mBins[bIdx];
							u64 baseMaskIdx = stepIdx;
							int MaskIdx = 0;

							if (bin.mIdx.size() > 0)
							{
								//copy bit locations in which all OPRF values are distinct

								//	Log::out << "    c_mPos= ";

								if (bin.mBits[IdxP].mPos.size() != bins.mSimpleBins.mNumBits[bIdxType])
								{
#ifdef PRINT
									Log::out << "bin.mBits[IdxP].mPos.size() != bins.mSimpleBins.mNumBits[bIdxType]" << Log::endl;
									Log::out << "Party: " << IdxP << Log::endl;
									Log::out << "bIdx: " << bIdx << Log::endl;
									Log::out << "bin.mBits[IdxP].mPos.size(): " << bin.mBits[IdxP].mPos.size() << Log::endl;
									Log::out << "mSimpleBins.mNumBits[bIdxType]: " << bins.mSimpleBins.mNumBits[bIdxType] << Log::endl;
#endif // PRINT
									throw std::runtime_error("bin.mBits.mPos.size()!= mBins.mNumBits");

								}

								//copy bit positions
								for (u64 idxPos = 0; idxPos < bin.mBits[IdxP].mPos.size(); idxPos++)
								{
									//	Log::out << static_cast<int16_t>(bin.mBits[IdxP].mPos[idxPos]) << " ";
									memcpy(
										maskView[baseMaskIdx].data() + idxPos,
										(u8*)&bin.mBits[IdxP].mPos[idxPos], sizeof(u8));
								}
								//Log::out << Log::endl;


								for (u64 i = 0; i < bin.mIdx.size(); ++i)
								{
									u64 inputIdx = bin.mIdx[i];
									block encr = bin.mValOPRF[IdxP][i] ^ plaintexts[inputIdx];

									//Log::out << "    c_idx=" << inputIdx;
									//Log::out << "    c_OPRF=" << encr;
									//Log::out << "    c_Map=" << static_cast<int16_t>(bin.mBits.mMaps[i]);

									MaskIdx = bin.mBits[IdxP].mMaps[i] * maskSize + bins.mSimpleBins.mNumBits[bIdxType];

									memcpy(
										maskView[baseMaskIdx].data() + MaskIdx,
										(u8*)&encr,
										maskSize);

									//	Log::out << Log::endl;
								}

								//#####################
								//######Filling dummy mask
								//#####################

								for (u64 i = 0; i < bins.mSimpleBins.mMaxBinSize[bIdxType]; ++i)
								{
									if (std::find(bin.mBits[IdxP].mMaps.begin(), bin.mBits[IdxP].mMaps.end(), i) == bin.mBits[IdxP].mMaps.end())
									{
										MaskIdx = i* maskSize + bins.mSimpleBins.mNumBits[bIdxType];
										//	Log::out << "    cc_Map=" << i << Log::endl;
										memcpy(
											maskView[baseMaskIdx].data() + MaskIdx,
											(u8*)&ZeroBlock,  //make randome
											maskSize);
									}
								}
							}
							else //pad all dummy
							{
								//bit positions
								std::vector<u8> dummyPos;
								auto idxDummyPos = 0;
								while (dummyPos.size()<bins.mSimpleBins.mNumBits[bIdxType])
								{
									u64 rand = std::rand() % 128; //choose randome bit location
									if (std::find(dummyPos.begin(), dummyPos.end(), rand) == dummyPos.end())
									{
										dummyPos.push_back(rand);
										memcpy(
											maskView[baseMaskIdx].data() + idxDummyPos,
											(u8*)&rand, sizeof(u8));
										idxDummyPos++;
									}
								}

								for (u64 i = 0; i < bins.mSimpleBins.mMaxBinSize[bIdxType]; ++i)
								{
									MaskIdx = i* maskSize + bins.mSimpleBins.mNumBits[bIdxType];
									//	Log::out << "    cc_Map=" << i << Log::endl;
									memcpy(
										maskView[baseMaskIdx].data() + MaskIdx,
										(u8*)&ZeroBlock,  //make randome
										maskSize);

								}

							}


						}

#ifdef PRINT
						Log::out << "maskSize: ";
						for (size_t i = 0; i < maskView.size()[0]; i++)
						{
							for (size_t j = 0; j < mSimpleBins.mNumBits[bIdxType]; j++)
							{
								Log::out << static_cast<int16_t>(maskView[i][j]) << " ";
							}
							Log::out << Log::endl;

							for (size_t j = 0; j < mSimpleBins.mMaxBinSize[bIdxType]; j++) {
								auto theirMask = ZeroBlock;
								memcpy(&theirMask, maskView[i].data() + j*maskSize + mSimpleBins.mNumBits[bIdxType], maskSize);
								if (theirMask != ZeroBlock)
								{
									Log::out << theirMask << " " << Log::endl;
								}
							}
						}
#endif
						chl.asyncSend(std::move(sendMaskBuff));

					}
				}
				if (tIdx == 0) gTimer.setTimePoint("online.send.sendMask");

				//	otSend.check(chl);



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


#if 0
	void OPPRFReceiver::sendSecretSharing(u64 IdxP, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls)
	{
		if (plaintexts.size() != mN)
			throw std::runtime_error(LOCATION);


		//TODO: double check
		u64 maskSize = sizeof(block);// roundUpTo(mStatSecParam + 2 * std::log(mN) - 1, 8) / 8;
									 //u64 maskSize = 7;
		if (maskSize > sizeof(block))
			throw std::runtime_error("masked are stored in blocks, so they can exceed that size");



		std::vector<std::thread>  thrds(chls.size());
		// std::vector<std::thread>  thrds(1);        

		std::mutex mtx;


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

				//2 type of bins: normal bin in inital step + stash bin
				for (auto bIdxType = 0; bIdxType < 2; bIdxType++)
				{
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
						sendMaskBuff->resize(currentStepSize * (bins.mSimpleBins.mMaxBinSize[bIdxType] * maskSize + bins.mSimpleBins.mNumBits[bIdxType] * sizeof(u8)));
						auto maskView = sendMaskBuff->getMatrixView<u8>(bins.mSimpleBins.mMaxBinSize[bIdxType] * maskSize + bins.mSimpleBins.mNumBits[bIdxType] * sizeof(u8));

						for (u64 stepIdx = 0; stepIdx < currentStepSize; ++bIdx, ++stepIdx)
						{
							//Log::out << "Bin #" << bIdx << Log::endl;

							auto& bin = bins.mSimpleBins.mBins[bIdx];
							u64 baseMaskIdx = stepIdx;
							int MaskIdx = 0;

							if (bin.mIdx.size() > 0)
							{
								//copy bit locations in which all OPRF values are distinct

							//	Log::out << "    c_mPos= ";

								if (bin.mBits[IdxP].mPos.size() != bins.mSimpleBins.mNumBits[bIdxType])
								{
#ifdef PRINT
									Log::out << "bin.mBits[IdxP].mPos.size() != mSimpleBins.mNumBits[bIdxType]" << Log::endl;
									Log::out << "Party: " << IdxP << Log::endl;
									Log::out << "bIdx: " << bIdx << Log::endl;
									Log::out << "bin.mBits[IdxP].mPos.size(): " << bin.mBits[IdxP].mPos.size() << Log::endl;
									Log::out << "mSimpleBins.mNumBits[bIdxType]: " << mSimpleBins.mNumBits[bIdxType] << Log::endl;
#endif // PRINT
									throw std::runtime_error("bin.mBits.mPos.size()!= mBins.mNumBits");

								}

								//copy bit positions
								for (u64 idxPos = 0; idxPos < bin.mBits[IdxP].mPos.size(); idxPos++)
								{
							//		Log::out << static_cast<int16_t>(bin.mBits[IdxP].mPos[idxPos]) << " ";
									memcpy(
										maskView[baseMaskIdx].data() + idxPos,
										(u8*)&bin.mBits[IdxP].mPos[idxPos], sizeof(u8));
								}
							//	Log::out << Log::endl;


								for (u64 i = 0; i < bin.mIdx.size(); ++i)
								{
									u64 inputIdx = bin.mIdx[i];
									block encr = bin.mValOPRF[IdxP][i] ^ plaintexts[inputIdx];

									//Log::out << "    c_idx=" << inputIdx;
									//Log::out << "    c_OPRF=" << encr;
									//Log::out << "    c_Map=" << static_cast<int16_t>(bin.mBits.mMaps[i]);

									MaskIdx = bin.mBits[IdxP].mMaps[i] * maskSize + bins.mSimpleBins.mNumBits[bIdxType];

									memcpy(
										maskView[baseMaskIdx].data() + MaskIdx,
										(u8*)&encr,
										maskSize);

									//	Log::out << Log::endl;
								}

								//#####################
								//######Filling dummy mask
								//#####################

								for (u64 i = 0; i < bins.mSimpleBins.mMaxBinSize[bIdxType]; ++i)
								{
									if (std::find(bin.mBits[IdxP].mMaps.begin(), bin.mBits[IdxP].mMaps.end(), i) == bin.mBits[IdxP].mMaps.end())
									{
										MaskIdx = i* maskSize + bins.mSimpleBins.mNumBits[bIdxType];
										//	Log::out << "    cc_Map=" << i << Log::endl;
										memcpy(
											maskView[baseMaskIdx].data() + MaskIdx,
											(u8*)&ZeroBlock,  //make randome
											maskSize);
									}
								}
							}
							else //pad all dummy
							{
								//bit positions
								std::vector<u8> dummyPos;
								auto idxDummyPos = 0;
								while (dummyPos.size()<bins.mSimpleBins.mNumBits[bIdxType])
								{
									u64 rand = std::rand() % 128; //choose randome bit location
									if (std::find(dummyPos.begin(), dummyPos.end(), rand) == dummyPos.end())
									{
										dummyPos.push_back(rand);
										memcpy(
											maskView[baseMaskIdx].data() + idxDummyPos,
											(u8*)&rand, sizeof(u8));
										idxDummyPos++;
									}
								}

								for (u64 i = 0; i < bins.mSimpleBins.mMaxBinSize[bIdxType]; ++i)
								{
									MaskIdx = i* maskSize + bins.mSimpleBins.mNumBits[bIdxType];
									//	Log::out << "    cc_Map=" << i << Log::endl;
									memcpy(
										maskView[baseMaskIdx].data() + MaskIdx,
										(u8*)&ZeroBlock,  //make randome
										maskSize);

								}

							}


						}

#ifdef PRINT
						Log::out << "maskSize: ";
						for (size_t i = 0; i < maskView.size()[0]; i++)
						{
							for (size_t j = 0; j < bins.mSimpleBins.mNumBits[bIdxType]; j++)
							{
								Log::out << static_cast<int16_t>(maskView[i][j]) << " ";
							}
							Log::out << Log::endl;

							for (size_t j = 0; j < bins.mSimpleBins.mMaxBinSize[bIdxType]; j++) {
								auto theirMask = ZeroBlock;
								memcpy(&theirMask, maskView[i].data() + j*maskSize + bins.mSimpleBins.mNumBits[bIdxType], maskSize);
								if (theirMask != ZeroBlock)
								{
									Log::out << theirMask << " " << Log::endl;
								}
							}
						}
#endif
						chl.asyncSend(std::move(sendMaskBuff));

					}
				}
				if (tIdx == 0) gTimer.setTimePoint("online.send.sendMask");

				//	otSend.check(chl);

				if (tIdx == 0) gTimer.setTimePoint("online.send.finalMask");
#endif
#pragma endregion

			});
		}

		for (auto& thrd : thrds)
			thrd.join();
	}

	void OPPRFReceiver::hash2Bins(std::vector<block>& inputs, Channel & chl)
	{
		hash2Bins(inputs, { &chl });
	}

	void OPPRFReceiver::hash2Bins(std::vector<block>& inputs, const std::vector<Channel*>& chls)
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


		std::vector<std::thread>  thrds(chls.size());
		//  std::vector<std::thread>  thrds(1);

		// since we are going to do this in parallel, these objects will
		// be used for synchronization. specifically, when all threads are 
		// done inserting items into the bins, the future will be fulfilled 
		// and all threads will advance to performing the base OtPsi's
		std::atomic<u32>
			insertRemaining((u32)thrds.size());

		std::promise<void> insertProm;
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
			auto seed = mPrng.get<block>();
			thrds[tIdx] = std::thread([&, tIdx, seed]()
			{

				if (tIdx == 0) gTimer.setTimePoint("online.recv.thrdStart");

				auto& chl = *chls[tIdx];

				auto startIdx = tIdx     * mN / thrds.size();
				auto endIdx = (tIdx + 1) * mN / thrds.size();
#pragma region Hashing


				std::vector<AES> ncoInputHasher(mNcoInputBlkSize);
				for (u64 i = 0; i < ncoInputHasher.size(); ++i)
					ncoInputHasher[i].setKey(_mm_set1_epi64x(i) ^ mHashingSeed);

				//Log::out << "mHashingSeed: " << mHashingSeed << Log::endl;

				u64 phaseShift = log2ceil(mN) / 8;

				for (u64 i = startIdx; i < endIdx; i += 128)
				{
					auto currentStepSize = std::min(u64(128), mN - i);

					for (u64 hashIdx = 0; hashIdx < ncoInputHasher.size(); ++hashIdx)
					{
						ncoInputHasher[hashIdx].ecbEncBlocks(
							inputs.data() + i,
							currentStepSize,
							mNcoInputBuff[hashIdx].data() + i);
					}


					// since we are using random codes, lets just use the first part of the code 
					// as where each item should be hashed.
					//	std::vector<block> tempMaskBuff(currentStepSize);

					std::vector<block> tempMaskBuff(currentStepSize);
					std::vector<u64> tempIdxBuff(currentStepSize);
					CuckooHasher1::Workspace w(tempMaskBuff.size());
					MatrixView<u64> hashes(currentStepSize, mCuckooBins.mParams.mNumHashes[0]);

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

				// block until all items have been inserted. the last to finish will set the promise...
				if (--insertRemaining)
					insertFuture.get();
				else
					insertProm.set_value();




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
#endif

}
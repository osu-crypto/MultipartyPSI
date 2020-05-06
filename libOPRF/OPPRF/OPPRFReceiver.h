#pragma once

#include "Common/Defines.h"
#include "Network/Channel.h"
#include "NChooseOne/NcoOtExt.h"
#include "Hashing/CuckooHasher1.h"
#include "Hashing/SimpleHasher1.h"
#include "OPPRF/binSet.h"

namespace osuCrypto
{

    class OPPRFReceiver
    {
    public:
        OPPRFReceiver();
        ~OPPRFReceiver();
        
        //static const u64 CodeWordSize = 7;
        //static const u64 hasherStepSize;

		
        u64 mN, mParties, mStatSecParam, mNcoInputBlkSize;// , mOtMsgBlkSize;
        block mHashingSeed;
        std::vector<u64> mIntersection;

		//std::vector<std::vector<block>> mNcoInputBuff;

		std::vector<std::unique_ptr<NcoOtExtSender>> mOtSends;
        std::vector<std::unique_ptr<NcoOtExtReceiver>> mOtRecvs;
		//CuckooHasher1 mCuckooBins;
		//SimpleHasher1 mSimpleBins;
        PRNG mPrng;

		u64 mNumBFhashs = 40;
		u64 mBfSize;
		std::vector<AES> mBFHasher;
		
		void init(u32 opt, u64 numParties, u64 n, u64 statSecParam, u64 inputBitSize, Channel& chl0, u64 otCounts, NcoOtExtReceiver& otRecv, NcoOtExtSender& otSend, block seed, bool isOtherDirection=true);
		void init(u32 opt, u64 numParties, u64 n, u64 statSecParam, u64 inputBitSize, const std::vector<Channel*>& chls, u64 otCounts, NcoOtExtReceiver& ots, NcoOtExtSender& otSend, block seed, bool isOtherDirection=true);

	

		//void copy2Bins(CuckooHasher1& cuckooBins);

		void getOPRFkeys( u64 IdxTheirParty, binSet& bins, Channel& chl, bool isOtherDirectionGetOPRF=true);
		void getOPRFkeys(u64 IdxTheirParty, binSet& bins, const std::vector<Channel*>& chls, bool isOtherDirectionGetOPRF = true);

		void getOPRFkeysSeperatedandTable(u64 IdxTheirParty, binSet& bins, const std::vector<Channel*>& chls, bool isOtherDirectionGetOPRF = true);

		void getOPRFkeysSeperated(u64 IdxTheirParty, binSet& bins, const std::vector<Channel*>& chls, bool isOtherDirectionGetOPRF = true);

		void getOPRFkeysCombined(u64 IdxTheirParty, binSet& bins, const std::vector<Channel*>& chls, bool isOtherDirectionGetOPRF = true);
		

		void recvSS(u64 IdxTheirParty, binSet& bins, std::vector<block>& plaintexts, Channel& chl);
		void sendSS(u64 IdxTheirParty, binSet& bins, std::vector<block>& plaintexts, Channel& chl);
		void sendSS(u64 IdxTheirParty, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls);
		void recvSS(u64 IdxTheirParty, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls);


		void recvSSTableBased(u64 IdxTheirParty, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls);
		void recvSSPolyBased(u64 IdxTheirParty, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls);
		void recvFullPolyBased(u64 IdxTheirParty, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls);
		void recvBFBased(u64 IdxTheirParty, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls);



		void sendSSTableBased(u64 IdxTheirParty, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls);
		void sendSSPolyBased(u64 IdxTheirParty, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls);
		void sendFullPolyBased(u64 IdxTheirParty, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls);
		void sendBFBased(u64 IdxTheirParty, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls);



    };




}

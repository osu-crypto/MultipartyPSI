#pragma once
#include "Common/Defines.h"
#include "Network/Channel.h"
#include "NChooseOne/NcoOtExt.h"
#include "Hashing/SimpleHasher1.h"
#include "Hashing/CuckooHasher1.h"
#include "Hashing/BitPosition.h"
#include "OPPRF/binSet.h"
namespace osuCrypto
{


    class OPPRFSender
    {
    public:


        //static const u64 CodeWordSize = 7;
        //static const u64 hasherStepSize;

        OPPRFSender();
        ~OPPRFSender();

		u64 mN, mParties, mStatSecParam, mNcoInputBlkSize,  mOtMsgBlkSize;
        block mHashingSeed;
		//SimpleHasher1 mSimpleBins;
		//CuckooHasher1 mCuckooBins;

		u64 mNumBFhashs = 40;
		u64 mBfSize;
		std::vector<AES> mBFHasher;

        PRNG mPrng;
		Timer mTimer;
		double mPosBitsTime=0;

		//std::vector<std::vector<block>> mmOPRF; //mValOPRF[bIdx][Idx]
		//std::vector<BaseOPPRF> mmBits;//mBits[bIdx]


        std::vector<std::unique_ptr<NcoOtExtSender>> mOtSends;
		std::vector<std::unique_ptr<NcoOtExtReceiver>> mOtRecvs;
		//std::vector<std::vector<block>> mNcoInputBuff;

        void init(u32 opt, u64 numParties, u64 setSize,  u64 statSecParam, u64 inputBitSize,
            const std::vector<Channel*>& chls, u64 otCounts,
            NcoOtExtSender& ots, 
			NcoOtExtReceiver& otRecv,
            block seed, bool isOtherDirection=true);

        void init(u32 opt,u64 numParties, u64 setSize,u64 statSecParam, u64 inputBitSize,
            Channel & chl0, u64 otCounts,
            NcoOtExtSender& ots,
			NcoOtExtReceiver& otRecv,
            block seed, bool isOtherDirection=true);

		void getOPRFkeys(u64 IdxTheirParty, binSet& bins, Channel& chl, bool isOtherDirectionGetOPRF = true);
		

		void getOPRFkeys(u64 IdxTheirParty, binSet& bins, const std::vector<Channel*>& chls, bool isOtherDirectionGetOPRF = false);

		void getOPRFkeysSeperatedandTable(u64 IdxTheirParty, binSet& bins, const std::vector<Channel*>& chls, bool isOtherDirectionGetOPRF = false);

		void getOPRFkeysSeperated(u64 IdxTheirParty, binSet& bins, const std::vector<Channel*>& chls, bool isOtherDirectionGetOPRF = false);
		
		void getOPRFkeysCombined(u64 IdxTheirParty, binSet& bins, const std::vector<Channel*>& chls, bool isOtherDirectionGetOPRF = false);

		void sendSS(u64 IdxTheirParty, binSet& bins, std::vector<block>& plaintexts, Channel& chl);
		void recvSS(u64 IdxTheirParty, binSet& bins, std::vector<block>& plaintexts, Channel& chl);
		void sendSS(u64 IdxTheirParty, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls);
		void recvSS(u64 IdxTheirParty, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls);

		void sendSSTableBased(u64 IdxTheirParty, binSet& bins, std::vector<block>& plaintexts,  const std::vector<Channel*>& chls);
		void sendSSPolyBased(u64 IdxTheirParty, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls);
		void sendFullPolyBased(u64 IdxTheirParty, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls);
		void sendBFBased(u64 IdxTheirParty, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls);


		void recvSSTableBased(u64 IdxTheirParty, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls);
		void recvSSPolyBased(u64 IdxTheirParty, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls);
		void recvFullPolyBased(u64 IdxTheirParty, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls);
		void recvBFBased(u64 IdxTheirParty, binSet& bins, std::vector<block>& plaintexts, const std::vector<Channel*>& chls);


    };

}
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

        PRNG mPrng;
		Timer mTimer;
		double mTime=0;

        std::vector<std::unique_ptr<NcoOtExtSender>> mOtSends;
		std::vector<std::unique_ptr<NcoOtExtReceiver>> mOtRecvs;
		//std::vector<std::vector<block>> mNcoInputBuff;

        void init(u64 numParties, u64 setSize,  u64 statSecParam, u64 inputBitSize,
            const std::vector<Channel*>& chls, 
            NcoOtExtSender& ots, 
			NcoOtExtReceiver& otRecv,
            block seed);

        void init(u64 numParties, u64 setSize,u64 statSecParam, u64 inputBitSize,
            Channel & chl0, 
            NcoOtExtSender& ots,
			NcoOtExtReceiver& otRecv,
            block seed);

		

		void getOPRFKeys( u64 IdxParty, binSet& bins, Channel& chl);
		void getOPRFKeys(u64 IdxParty, binSet& bins, const std::vector<Channel*>& chls);

			void sendSecretSharing(u64 IdxParty, binSet& bins, std::vector<block>& plaintexts,  Channel& chl);
		void sendSecretSharing(u64 IdxParty, binSet& bins, std::vector<block>& plaintexts,  const std::vector<Channel*>& chls);
#if 0
		void revSecretSharing(u64 IdxParty, std::vector<block>& plaintexts, Channel& chl);
		void revSecretSharing(u64 IdxParty, std::vector<block>& plaintexts, const std::vector<Channel*>& chls);
		void hash2Bins(std::vector<block>& inputs, Channel& chl);
		void hash2Bins(std::vector<block>& inputs, const std::vector<Channel*>& chls);

#endif
    };

}
#pragma once

#include "Common/Defines.h"
#include "Network/Channel.h"
#include "NChooseOne/NcoOtExt.h"
#include "Hashing/CuckooHasher1.h"
#include "Hashing/SimpleHasher1.h"

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

		std::vector<std::vector<block>> mNcoInputBuff;

		std::vector<std::unique_ptr<NcoOtExtSender>> mOtSends;
        std::vector<std::unique_ptr<NcoOtExtReceiver>> mOtRecvs;

		CuckooHasher1 mCuckooBins;
		SimpleHasher1 mSimpleBins;
        PRNG mPrng;

		void init(u64 numParties, u64 n, u64 statSecParam, u64 inputBitSize, Channel& chl0, NcoOtExtReceiver& otRecv, NcoOtExtSender& otSend, block seed);
		void init(u64 numParties, u64 n, u64 statSecParam, u64 inputBitSize, const std::vector<Channel*>& chls, NcoOtExtReceiver& ots, NcoOtExtSender& otSend, block seed);

		void hash2Bins(std::vector<block>& inputs, Channel& chl);
		void hash2Bins(std::vector<block>& inputs, const std::vector<Channel*>& chls);

		void getOPRFkeys( u64 IdxParty, Channel& chl);
		void getOPRFkeys(u64 IdxParty, const std::vector<Channel*>& chls);
#if 0
		void decrypt( std::vector<block>& plaintexts, Channel& chl);
		void decrypt( std::vector<block>& plaintexts, const std::vector<Channel*>& chls);
#endif

    };




}

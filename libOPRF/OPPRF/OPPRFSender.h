#pragma once
#include "Common/Defines.h"
#include "Network/Channel.h"
#include "NChooseOne/NcoOtExt.h"
#include "Hashing/SimpleHasher1.h"
#include "Hashing/CuckooHasher1.h"
#include "Hashing/BitPosition.h"

namespace osuCrypto
{


    class OPPRFSender
    {
    public:


        //static const u64 CodeWordSize = 7;
        //static const u64 hasherStepSize;

        OPPRFSender();
        ~OPPRFSender();

        u64 mN, mStatSecParam, mNcoInputBlkSize,  mOtMsgBlkSize;
        block mHashingSeed;
		SimpleHasher1 mSimpleBins;
		CuckooHasher1 mCuckooBins;

        PRNG mPrng;

        std::vector<std::unique_ptr<NcoOtExtSender>> mOtSends;
		std::vector<std::unique_ptr<NcoOtExtReceiver>> mOtRecvs;
		std::vector<std::vector<block>> mNcoInputBuff;

        void init(u64 n, u64 statSecParam, u64 inputBitSize, 
            const std::vector<Channel*>& chls, 
            NcoOtExtSender& ots, 
			NcoOtExtReceiver& otRecv,
            block seed);

        void init(u64 n, u64 statSecParam, u64 inputBitSize, 
            Channel & chl0, 
            NcoOtExtSender& ots,
			NcoOtExtReceiver& otRecv,
            block seed);

		void hash2Bins(std::vector<block>& inputs, Channel& chl);
		void hash2Bins(std::vector<block>& inputs, const std::vector<Channel*>& chls);
#if 0
		void getOPRFKeys(std::vector<block>& inputs, Channel& chl);
		void getOPRFKeys(std::vector<block>& inputs, const std::vector<Channel*>& chls);

		void sendEnc(std::vector<block>& plaintexts,  Channel& chl);
		void sendEnc(std::vector<block>& plaintexts,  const std::vector<Channel*>& chls);

#endif
    };

}
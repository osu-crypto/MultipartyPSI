#pragma once
#include "Common/Defines.h"
#include "Network/Channel.h"
#include "NChooseOne/NcoOtExt.h"
#include "Hashing/SimpleHasher1.h"

namespace osuCrypto
{


    class BarkOPRFSender
    {
    public:


        //static const u64 CodeWordSize = 7;
        //static const u64 hasherStepSize;

        BarkOPRFSender();
        ~BarkOPRFSender();

        u64 mN, mStatSecParam, mNcoInputBlkSize,  mOtMsgBlkSize;
        block mHashingSeed;
        SimpleHasher1 mBins;
        PRNG mPrng;

        std::vector<std::unique_ptr<NcoOtExtSender>> mOtSends;

        void init(u64 n, u64 statSecParam, u64 inputBitSize, 
            const std::vector<Channel*>& chls, 
            NcoOtExtSender& ots, 
            block seed);

        void init(u64 n, u64 statSecParam, u64 inputBitSize, 
            Channel & chl0, 
            NcoOtExtSender& ots,
            block seed);

        void sendInput(std::vector<block>& inputs, Channel& chl);
        void sendInput(std::vector<block>& inputs,const std::vector<Channel*>& chls);

    };

}
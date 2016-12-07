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

        u64 mN, mStatSecParam, mNcoInputBlkSize;// , mOtMsgBlkSize;
        block mHashingSeed;
        std::vector<u64> mIntersection;


        std::vector<std::unique_ptr<NcoOtExtReceiver>> mOtRecvs;

        CuckooHasher1 mBins;
		//SimpleHasher1 mTheirBins;
        PRNG mPrng;

        void init(u64 n, u64 statSecParam, u64 inputBitSize, Channel& chl0, NcoOtExtReceiver& otRecv, block seed);
        void init(u64 n, u64 statSecParam, u64 inputBitSize, const std::vector<Channel*>& chls, NcoOtExtReceiver& ots, block seed);
        void sendInput(std::vector<block>& inputs, Channel& chl);
        void sendInput(std::vector<block>& inputs, const std::vector<Channel*>& chls);

    };




}

#pragma once

#include "Common/Defines.h"
#include "Network/Channel.h"
#include "NChooseOne/NcoOtExt.h"
#include "Hashing/CuckooHasher1.h"
#include "Hashing/SimpleHasher1.h"
#include "OPPRF/OPPRFReceiver.h"
#include "OPPRF/OPPRFSender.h"

namespace osuCrypto
{

    class binSet
    {
    public:
		binSet();
        ~binSet();
        
        //static const u64 CodeWordSize = 7;
        //static const u64 hasherStepSize;

        u64 mN, mParties, mMyIdx, mStatSecParam, mNcoInputBlkSize;// , mOtMsgBlkSize;
        block mHashingSeed;

		std::vector<std::vector<block>> mNcoInputBuff;

		//std::vector<OPPRFSender> mOpprfSends;
        //std::vector<OPPRFReceiver> mOpprfRecvs;

		CuckooHasher1 mCuckooBins;
		SimpleHasher1 mSimpleBins;
      

		void init(u64 myIdx, u64 nParties, u64 setSize, u64 statSecParam);

		void hashing2Bins(std::vector<block>& inputs, int numThreads);
    };

}

#pragma once

#include "Common/Defines.h"
#include "Network/Channel.h"
#include "NChooseOne/NcoOtExt.h"
#include "Hashing/CuckooHash.h"
#include "Hashing/SimpleHash.h"
#include "Parameters.h"

namespace osuCrypto
{

	//struct BFParam
	//{
	//	double mBinScaler[2]; //first index is for init step, 2nd index for stash step
	//	u64 mNumHashes[2];
	//	u64 mSenderBinSize[2];
	//	/*
	//	double mBinStashScaler;
	//	u64 mNumStashHashes;
	//	u64 mSenderBinStashSize;*/
	//};

    class binSet
    {
    public:
		binSet();
        ~binSet();
		
		//const u64 stepSize = 16;

        u64 mN,mTheirN, mParties, mMyIdx, mStatSecParam, mNcoInputBlkSize;// , mOtMsgBlkSize;
        block mHashingSeed;
		u64 mMaskSize;
		u64 mOpt;

		std::vector<std::vector<block>> mNcoInputBuff; //hash(x)

	//	OPPRFSender aaa;
		std::vector<block> mXsets;
		

		CuckooHash mCuckooBins;
		SimpleHash mSimpleBins;
      

		void init(u64 myIdx, u64 nParties, u64 mySetSize, u64 theirSetSize, u64 statSecParam, u64 opt);

		void hashing2Bins(std::vector<block>& inputs, int numThreads);
    };

}

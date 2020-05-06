#pragma once

#include "Common/Defines.h"
#include "Network/Channel.h"
#include "NChooseOne/NcoOtExt.h"
#include "Hashing/CuckooHasher1.h"
#include "Hashing/SimpleHasher1.h"

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
		

        u64 mN, mParties, mMyIdx, mStatSecParam, mNcoInputBlkSize;// , mOtMsgBlkSize;
        block mHashingSeed;
		u64 mMaskSize;
		u64 mOpt;

		std::vector<std::vector<block>> mNcoInputBuff; //hash(x)

	//	OPPRFSender aaa;
		std::vector<block> mXsets;
		

		CuckooHasher1 mCuckooBins;
		SimpleHasher1 mSimpleBins;
      

		void init(u64 myIdx, u64 nParties, u64 setSize, u64 statSecParam, u64 opt);

		void hashing2Bins(std::vector<block>& inputs, int numThreads);
    };

}

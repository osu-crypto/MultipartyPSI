#pragma once
#include "Common/Defines.h"
#include "Common/BitVector.h"
#include "Common/ArrayView.h"
#include "Hashing/Hints.h"
namespace osuCrypto
{
	//// a list of {{set size, bit size}}
	//std::vector<std::array<u64, 2>> binSizes
	//{
	//    {1<<12, 18},
	//    {1<<16, 19},
	//    {1<<20, 20},
	//    {1<<24, 21}
	//};



	struct SimpleParams
	{

		double mBinScaler[2];
		u64 mNumHashes[2];

		u64 mMaxBinSize[2];
		u64 mNumBits[2];
		/*
		double mBinStashScaler;
		u64 mNumStashHashes;
		u64 mSenderBinStashSize;*/
	};

	class SimpleHash
	{
	public:
		SimpleHash();
		~SimpleHash();

		
		//typedef std::vector<u64,block> MtBin;
		//typedef std::vector<std::pair<u64, block>> MtBin;
		struct Bin
		{
			std::vector<u64> mIdx; //have many items in the bin
								   //hash index used for mIdx. Only use when combined hints
								   //one can think mIdx and hIdx as a pair <mIdx,hIdx>....
			std::vector<u64> hIdx;
			std::vector<TableBased> mBits;//mBits[IdxParty]
			std::vector<std::vector<block>> mValOPRF; //mValOPRF[IdxParty][mIdx]
		};
		u64  mRepSize, mInputBitSize, mN;
		u64 mBinCount[2], mMaxBinSize[2], mNumHashes[2], mNumBits[2];

		//remember hIdx for the combined keys
		//mOpprfs[IdxParty][inputIdx][hIdx]
		std::vector<std::vector<std::vector<block>>> mOprfs;
		u64 testMaxBinSize;
		std::vector<u64> realBinSizeCount1;
		std::vector<u64> realBinSizeCount2;

		std::unique_ptr<std::mutex[]> mMtx;
		std::vector<Bin> mBins;
		block mHashSeed;
		SimpleParams mParams;
		void print(u64 idxParty, bool isIdx, bool isOPRF, bool isMap, bool isPos, u64 opt = 0) const;

		u64 maxRealBinSize();

		void init(u64 n, u64 theirN, u64 opt);

		void insertBatch(
			ArrayView<u64> inputIdxs,
			MatrixView<u64> hashs);

		//void preHashedInsertItems(ArrayView<block> items, u64 itemIdx);
		//void insertItemsWithPhasing(ArrayView<block> items, u64 itemIdx);
	};

}

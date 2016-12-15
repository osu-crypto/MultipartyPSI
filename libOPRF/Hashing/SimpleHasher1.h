#pragma once
#include "Common/Defines.h"
#include "Common/BitVector.h"
#include "Common/ArrayView.h"
#include "Hashing/BitPosition.h"
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


    class SimpleHasher1
    {
    public:
        SimpleHasher1();
        ~SimpleHasher1();

        //typedef std::vector<u64,block> MtBin;
        //typedef std::vector<std::pair<u64, block>> MtBin;
		struct Bin
		{
			std::vector<u64> mIdx;
			BitPosition mBits;
			std::vector<block> mValOPRF;
		};
        u64 mBinCount , mMaxBinSize, mRepSize, mInputBitSize, mN, mNumHashes, mNumBits;

        std::unique_ptr<std::mutex[]> mMtx;
        std::vector<Bin> mBins;
        block mHashSeed;

        void print(bool isIdx, bool isOPRF, bool isMap, bool isPos) const;

        void init(u64 n, u64 numBits, block hashSeed, u64 secParam, bool isStash);

		void insertBatch(
			ArrayView<u64> inputIdxs,
			MatrixView<u64> hashs,
			u64 numHashFunc);

        //void preHashedInsertItems(ArrayView<block> items, u64 itemIdx);
        //void insertItemsWithPhasing(ArrayView<block> items, u64 itemIdx);
    };

}

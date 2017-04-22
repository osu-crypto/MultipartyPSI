#pragma once
#include "Common/Defines.h"
#include "Common/Log.h"
#include "Common/BitVector.h"
#include "Common/ArrayView.h"
#include "Common/MatrixView.h"
#include "Hashing/BitPosition.h"
//#include <mutex>
#include <atomic>

//#define THREAD_SAFE_CUCKOO

namespace osuCrypto
{
    struct CuckooParam1
    {
        double mBinScaler[2]; //first index is for init step, 2nd index for stash step
		u64 mNumHashes[2];
		u64 mSenderBinSize[2];
/*
		double mBinStashScaler;
		u64 mNumStashHashes;
		u64 mSenderBinStashSize;*/
    };



    class CuckooHasher1
    {
    public:
        CuckooHasher1();
        ~CuckooHasher1();

        struct Bin
        {
            Bin() :mVal(-1) {}
            Bin(u64 idx, u64 hashIdx) : mVal(idx | (hashIdx << 56)) {}

            bool isEmpty() const;
            u64 idx() const;
            u64 hashIdx() const;

            void swap(u64& idx, u64& hashIdx);
#ifdef THREAD_SAFE_CUCKOO
            Bin(const Bin& b) : mVal(b.mVal.load(std::memory_order_relaxed)) {}
            Bin(Bin&& b) : mVal(b.mVal.load(std::memory_order_relaxed)) {}
            std::atomic<u64> mVal;
#else
            Bin(const Bin& b) : mVal(b.mVal) {}
            Bin(Bin&& b) : mVal(b.mVal) {}
            u64 mVal;
			std::vector<block> mValOPRF;
			std::vector<u8> mValMap;

			std::vector<std::vector<block>> mCoeffs;//mBits[IdxParty][mIdx]
#endif
        };
        struct Workspace
        {
            Workspace(u64 n)
                : curAddrs(n)
                , curHashIdxs(n)
                , oldVals(n)
                //, findAddr(n)
                , findVal(n)
            {}

            std::vector<u64>
                curAddrs,// (inputIdxs.size(), 0),
                curHashIdxs,// (inputIdxs.size(), 0),
                oldVals;// (inputIdxs.size());

            std::vector<std::array<u64, 2>> /*findAddr,*/ findVal;
        };


		std::mutex mInsertBin;
        u64 mTotalTries;

        bool operator==(const CuckooHasher1& cmp)const;
        bool operator!=(const CuckooHasher1& cmp)const;

        //std::mutex mStashx;

        CuckooParam1 mParams;
		block mHashSeed;
		u64  mRepSize, mInputBitSize, mN;
		u64 mBinCount[2];//mBinCount[0] for init step, mBinCount[1] for Stash step
        void print(u64 IdxParty, bool isIdx, bool isOPRF, bool isMap) const;
		void init(u64 n,u64 opt);
        void insert(u64 IdxItem, ArrayView<u64> hashes);
        void insertHelper(u64 IdxItem, u64 hashIdx, u64 numTries);
		void insertStashHelper(u64 IdxItem, u64 hashIdx, u64 numTries);

        void insertBatch(ArrayView<u64> itemIdxs, MatrixView<u64> hashs, Workspace& workspace);

		
			void insertStashBatch(ArrayView<u64> itemIdxs, MatrixView<u64> hashs, Workspace& workspace);


        u64 find(ArrayView<u64> hashes);
        u64 findBatch(MatrixView<u64> hashes, 
            ArrayView<u64> idxs,
            Workspace& wordkspace);

   // private:

        std::vector<u64> mHashes;
        MatrixView<u64> mHashesView;

		std::vector<u64> mStashHashes;
		MatrixView<u64> mStashHashesView;

        std::vector<Bin> mBins;
        std::vector<Bin> mStashBins;
		std::vector<u64> mStashIdxs;

        //std::vector<Bin> mBins;
        //std::vector<Bin> mStash;


        //void insertItems(std::array<std::vector<block>,4>& hashs);
    };

}

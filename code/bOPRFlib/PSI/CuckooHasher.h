#pragma once
#include "Common/Defines.h"
#include "Common/BitVector.h"
#include "Common/ArrayView.h"

namespace bOPRF
{	
	class CuckooHasher
	{
	public:
		CuckooHasher();
		~CuckooHasher();

		struct Bin
		{
			Bin()
				:mIdx(-1),
				mHashIdx(0)
			{}

			Bin(u64 i, u64 v)
				: mIdx(i)
				, mHashIdx(v) {}

			u64 mIdx;
			u64 mHashIdx;


			bool isEmpty() const
			{
				return mIdx == u64(-1);
			}
		}; 


		u64 mBinCount, mN, mMaxStashSize, mSendersMaxBinSize, mTotalTries;

		std::vector<Bin> mBins;
		std::vector<Bin> mStash;
		
		void print() const;

		void init(u64 n);
		void insertItem(u64 IdxItem, std::array<std::vector<block>, 4>& hashs, u64 hashIdx = 0, u64 numTries = 0);
		void insertItems(std::array<std::vector<block>,4>& hashs);
	};

}

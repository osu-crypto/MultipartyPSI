#pragma once
#include "Common/Defines.h"
#include "Common/BitVector.h"
#include "Common/ArrayView.h"

namespace bOPRF
{
	
	class SimpleHasher
	{
	public:
		SimpleHasher();
		~SimpleHasher();

		struct item
		{
			u64 mIdx;
			u64 mHashIdx;
		};

		struct Bin
		{

			Bin() :mSize(0) {}

			std::array<item, 21> mItems;
			u64 mSize;

		};


		u64 mBinCount, mMaxBinSize, mN;
		std::vector<Bin> mBins;
		block mHashSeed;
		void print() const;

	
		void init(u64 n); 
		u64 insertItems(std::array<std::vector<block>,4> hashs);

	};

}

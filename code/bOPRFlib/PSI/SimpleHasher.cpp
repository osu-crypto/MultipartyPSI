#include "SimpleHasher.h"
#include "Crypto/sha1.h"
#include "Crypto/PRNG.h"
#include <random>
#include "Common/Log.h"
#include <numeric>
#include <algorithm>

namespace bOPRF
{


	SimpleHasher::SimpleHasher()
	{
	}


	SimpleHasher::~SimpleHasher()
	{
	}

	void SimpleHasher::print() const
	{

		Log::out << Log::lock;
		/*for(u64 j=0;j<3;j++)
			Log::out << "  mHashSeeds[" << j << "] " << mHashSeeds[j] << Log::endl;*/
		Log::out << "Simple Hasher  " << Log::endl;
		for (u64 i = 0; i < mBins.size(); ++i)
			//	for (u64 i = 0; i <1; ++i)
		{
			Log::out << "Bin #" << i << Log::endl;

			Log::out << " contains " << mBins[i].mSize << " elements" << Log::endl;

			for (u64 j = 0; j < mBins[i].mSize; ++j)
			{
				Log::out << "    idx=" << mBins[i].mItems[j].mIdx << "  hIdx=" << mBins[i].mItems[j].mHashIdx << Log::endl;
				//	Log::out << "    " << mBins[i].first[j] << "  " << mBins[i].second[j] << Log::endl;

			}

			Log::out << Log::endl;
		}

		Log::out << Log::endl;
		Log::out << Log::unlock;
	}

	void SimpleHasher::init(u64 n)
	{		
		mN = n;
		mBinCount = 1.2*n;
		mMaxBinSize = get_bin_size(n);;
		mBins.resize(mBinCount);
	}


	u64 SimpleHasher::insertItems(std::array<std::vector<block>, 4> hashs)
	{


		u64 cnt=0;
		u8 xrHash[SHA1::HashSize];

		SHA1 f;

		for (u64 i = 0; i < hashs[0].size(); ++i)
		{
			//Log::out << "  item[" << i << "] " << item << Log::endl;

			u64 addr;
			//std::array<u64, 2> idxs{ -1,-1 };
			std::array<u64, 3> idxs{ -1,-1,-1 };

			for (u64 j = 0; j < 3; ++j) {

				u64 xrHashVal = *(u64*)&hashs[j][i] % mBinCount;

				addr = xrHashVal % mBinCount;
				//cnt1++;

				//if (std::find(idxs.begin(), idxs.end(), addr ) == idxs.end())
				{
					auto bb = mBins[addr].mSize;
					//mBins[addr].mItems[bb].mValue = item;
					mBins[addr].mItems[bb].mIdx  =i;
					mBins[addr].mItems[bb].mHashIdx=j;

					mBins[addr].mSize++;
					cnt++;

					//idxs[j] = addr;
				}
			}
		}	
		return cnt;
	}

}

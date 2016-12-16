#include "SimpleHasher1.h"
#include "Crypto/sha1.h"
#include "Crypto/PRNG.h"
#include <random>
#include "Common/Log.h"
#include "Common/Log1.h"
#include <numeric>

namespace osuCrypto
{


	SimpleHasher1::SimpleHasher1()
	{
	}


	SimpleHasher1::~SimpleHasher1()
	{
	}



	void SimpleHasher1::print(u64 IdxP, bool isIdx, bool isOPRF, bool isMap, bool isPos) const
	{

		std::cout << IoStream::lock;
		int cnt = 0;
		//std::cout << "SimpleHasher1  " << std::endl;
		Log::out << "SimpleHasher1  " << Log::endl;
		//for (u64 i = 0; i < 10; ++i)
		for (u64 i = 0; i < mBins.size(); ++i)
		{
			//std::cout << "Bin #" << i << std::endl;
			Log::out << "Bin #" << i << ": " << Log::endl;

			//std::cout << " contains " << mBins[i].size() << " elements" << std::endl;
			//Log::out << " contains " << mBins[i].size() << " elements" << Log::endl;
			if (mBins[i].mIdx.size() > 0)

				if (isOPRF && mBins[i].mIdx.size() != mBins[i].mValOPRF[IdxP].size())
				{
					Log::out << "mBins[i].mIdx.size() != mBins[i].mValOPRF.size()" << Log::endl;
				}
				else if (mBins[i].mIdx.size() != mBins[i].mBits[IdxP].mMaps.size() && isMap) {
					Log::out << "mBins[i].mIdx.size() != mBins[i].mMaps.size()" << Log::endl;
				}

				else if (i<mBinCount && mBins[i].mBits[IdxP].mPos.size() != mNumBits && isPos)
					Log::out << "mBins[i].mBits.mPos.size() != mNumBits" << Log::endl;

				else if (i>mBinCount-1 && mBins[i].mBits[IdxP].mPos.size() != mNumStashBits && isPos)
					Log::out << "mBins[i].mBits.mPos.size() != mNumBits" << Log::endl;

				else
				{
					if (isPos) {
						Log::out << "    c_Pos= ";
						for (u64 j = 0; j < mBins[i].mBits[IdxP].mPos.size(); j++)
						{
							Log::out << static_cast<int16_t>(mBins[i].mBits[IdxP].mPos[j]) << " ";
						}
						Log::out << Log::endl;
					}

					for (u64 j = 0; j < mBins[i].mIdx.size(); ++j)
					{
						if (isIdx)
							Log::out << "    c_idx=" << mBins[i].mIdx[j];

						if (isOPRF)
							Log::out << "    c_OPRF=" << mBins[i].mValOPRF[IdxP][j];

						if (isMap)
							Log::out << "    c_Map=" << static_cast<int16_t>(mBins[i].mBits[IdxP].mMaps[j]);

						Log::out << Log::endl;
						cnt++;
					}

				}

			/*for (auto iter = mBins[i].mBits.mMasks.begin(); iter != mBins[i].mBits.mMasks.end(); ++iter) {
				std::cout << static_cast<int16_t>((*iter)) << " ";
			}
*/
//std::cout << std::endl;
			Log::out << Log::endl;
		}

		//std::cout << "cnt" << cnt << std::endl;
		Log::out << Log::endl;
		std::cout << IoStream::unlock;
	}

	double maxprob1(u64 balls, u64 bins, u64 k)
	{
		return std::log(bins * std::pow(balls * exp(1) / (bins * k), k)) / std::log(2);
	}

	void SimpleHasher1::init(u64 numParties, u64 n, u64 numBits, block hashSeed, u64 secParam)
	{

		mHashSeed = hashSeed;
		mN = n;

		auto log2n = log2ceil(n);

		mInputBitSize = numBits;

#if 0
		for (u64 maxBin = 15; maxBin < 64; maxBin++)
		{
			u64 binsHigh = n * 2;
			u64 binsLow = 1;
			// finds the min number of bins needed to get max occ. to be maxBin

			if (-maxprob1(n, binsHigh, maxBin) < secParam)
			{
				// maxBins is too small, skip it.
				continue;
			}


			while (binsHigh != binsLow && binsHigh - 1 != binsLow)
			{
				auto mid = (binsHigh + binsLow) / 2;

				if (-maxprob1(n, mid, maxBin) < secParam)
				{
					binsLow = mid;
				}
				else
				{
					binsHigh = mid;
				}
			}

			u64 bins = binsHigh;

			u64 logBinCount = (u64)std::log2(bins);

			double total = bins*(double)maxBin * (double)maxBin * ((double)mInputBitSize - logBinCount);

			if (total < best)
			{
				best = total;
				mBinCount = bins;
				mMaxBinSize = maxBin;
				//std::cout << "##########################################################" << std::endl;
				//std::cout << n << "  " << bins << "   " << maxBin << "    " << logBinCount << "     " << total << std::endl;
				//std::cout << "##########################################################" << std::endl;

			}
		}
#endif
		mMaxBinSize = 32;
		mMaxBinStashSize = 64;

		mBinCount = 1.2*n;
		mBinStashCount = 0.3*n;

		mMtx.reset(new std::mutex[mBinCount + mBinStashCount]);
		mBins.resize(mBinCount + mBinStashCount);
		mNumHashes = 3;
		mNumStashHashes = 3;
		mNumBits = 5;
		mNumStashBits = 6;

	}

	void SimpleHasher1::insertBatch(ArrayView<u64> inputIdxs, MatrixView<u64> hashs)
	{
		for (u64 j = 0; j < inputIdxs.size(); ++j)
		{
			for (u64 k = 0; k < mNumHashes; ++k)
			{
				u64 addr = *(u64*)&hashs[j][k] % mBinCount;
				//if(addr==0)
				//	std::cout << "----"<<inputIdxs[j] <<"-" << addr << std::endl;

				std::lock_guard<std::mutex> lock(mMtx[addr]);
				if (std::find(mBins[addr].mIdx.begin(), mBins[addr].mIdx.end(), inputIdxs[j]) == mBins[addr].mIdx.end()) {
					mBins[addr].mIdx.emplace_back(inputIdxs[j]);
					//		std::cout << "1----"<<inputIdxs[j] <<"-" << addr << std::endl;
				}
			}

			for (u64 k = 0; k < mNumStashHashes; ++k)
			{
				u64 addrStash = *(u64*)&hashs[j][k] % mBinStashCount + mBinCount;
				std::lock_guard<std::mutex> lock(mMtx[addrStash]);
				if (std::find(mBins[addrStash].mIdx.begin(), mBins[addrStash].mIdx.end(), inputIdxs[j]) == mBins[addrStash].mIdx.end()) {
					mBins[addrStash].mIdx.emplace_back(inputIdxs[j]);
					//	std::cout << "2----" << inputIdxs[j] << "-" << addrStash << std::endl;

				}
			}

		}
	}

	//void SimpleHasher1::preHashedInsertItems(ArrayView<block> mySet, u64 itemIdx)
	//{
	//    for (u64 i = 0; i < mySet.size(); ++i, ++itemIdx)
	//    {
	//        auto& item = mySet[i];


	//        u64 addr = *(u64*)&item % mBinCount;

	//        std::lock_guard<std::mutex> lock(mMtx[addr]);
	//        mBins[addr].emplace_back();
	//        mBins[addr].back() = itemIdx;
	//    }
	//}

	////void SimpleHasher1::insertItemsWithPhasing(
	//    ArrayView<block> mySet,  
	//    u64 itemIdx)
	//{
	//    u64 addressbitSize = mInputBitSize - mRepSize;
	//    throw std::runtime_error("not impl");

	//    //SHA1 fSeed;
	//    //fSeed.Update(mHashSeed);
	//    //std::cout << "hash seed     " << mHashSeed << std::endl;
	//    //std::cout << "mInputBitSize " << mInputBitSize << std::endl;
	//    //std::cout << "mRepSize      " << mRepSize << std::endl;
	//    //std::cout << "totalRepSize  " << totalRepSize << std::endl << std::endl;

	//    u8 xrHash[SHA1::HashSize];

	//    SHA1 f;

	//    //for (u64 i = 0; i < mySet.size(); ++i)
	//    //{
	//    //    auto& item = mySet[i];
	//    //    //std::cout << "  item[" << i << "] " << item << std::endl;

	//    //    //u64 xr(0),xl(0);

	//    //    //memccpy(&xr, &item, )

	//    //    //BitVector xr;
	//    //    //xr.append((u8*)&item, mRepSize);

	//    //    //BitVector xl;
	//    //    //xl.append((u8*)&item, addressbitSize, xr.size());
	//    //    auto xr = item / mBinCount;
	//    //    auto xl = item % mBinCount;

	//    //    //auto f = fSeed;

	//    //    f.Reset();
	//    //    f.Update(mHashSeed);
	//    //    f.Update((u8*)&xr, sizeof(u64));
	//    //    f.Final(xrHash);

	//    //    //u64 xlVal = 0;
	//    //    //memcpy(&xlVal, xl.data(), xl.sizeBytes());

	//    //    u64 xrHashVal = *(u64*)xrHash % mBinCount;

	//    //    auto addr = (xl + xrHashVal) % mBinCount;

	//    //    //std::cout << "     xr   " << xr << std::endl;
	//    //    //std::cout << "     xl   " << xl << std::endl;
	//    //    //std::cout << "     addr " << addr <<  std::endl;

	//    //    BitVector val(mRepSize);
	//    //    memcpy(val.data(), &xr, std::min(sizeof(xr), val.sizeBytes()));

	//    //    std::lock_guard<std::mutex> lock(mMtx[addr]);
	//    //    mBins[addr].first.push_back(i);
	//    //    mBins[addr].second.emplace_back(std::move(val));

	//    //    //if (i == 0)
	//    //    //{
	//    //    //    std::cout << IoStream::lock << item << "  -> addr = " << addr << "  val = " << xr << "  (" << mBins[addr].second.back() << ")" << std::endl << IoStream::unlock;
	//    //    //}
	//    //}
	//}
}

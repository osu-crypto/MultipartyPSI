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



    void SimpleHasher1::print() const
    {

        std::cout << IoStream::lock;
		//std::cout << "SimpleHasher1  " << std::endl;
		Log::out << "SimpleHasher1  " << Log::endl;
		//for (u64 i = 0; i < 10; ++i)
        for (u64 i = 0; i < mBins.size(); ++i)
        {
			//std::cout << "Bin #" << i << std::endl;
			Log::out << "Bin #" << i;

			//std::cout << " contains " << mBins[i].size() << " elements" << std::endl;
			//Log::out << " contains " << mBins[i].size() << " elements" << Log::endl;

            for (u64 j = 0; j < mBins[i].size(); ++j)
            {
               // std::cout
               //     << "    " << mBins[i][j]  
               //     /*<< "  " << mBins[i][j].second */<< std::endl;
				//Log::out
				//	<< "    " << mBins[i][j]
				//	/*<< "  " << mBins[i][j].second */ << Log::endl;

				Log::out << "    c_idx=" << mBins[i][j];
            }

			//std::cout << std::endl;
			Log::out <<  Log::endl;
        }

		//std::cout << std::endl;
		Log::out <<  Log::endl;
		std::cout << IoStream::unlock;
    }

    double maxprob1(u64 balls, u64 bins, u64 k)
    {
        return std::log(bins * std::pow(balls * exp(1) / (bins * k), k)) / std::log(2);
    }

    void SimpleHasher1::init(u64 n, u64 numBits, block hashSeed, u64 secParam)
    {
        mHashSeed = hashSeed;
        mN = n;

        auto log2n = log2ceil(n);

        mInputBitSize = numBits;

        double best = (999999999999999.0);
#if 0
        for (u64 maxBin = 15; maxBin < 40; maxBin++)
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
		mMaxBinSize = 40;
		mBinCount = 1.2*n;
        mMtx.reset(new std::mutex[mBinCount]);
        mBins.resize(mBinCount);
       // mRepSize = mInputBitSize - (u32)std::log2(mBinCount);
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

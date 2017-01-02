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
					Log::out << "mBins[i].mIdx.size()= "<< mBins[i].mIdx.size() << Log::endl;
					Log::out << "mBins[i].mValOPRF.size()= " << mBins[i].mValOPRF.size() << Log::endl;
				}
				else if (mBins[i].mIdx.size() != mBins[i].mBits[IdxP].mMaps.size() && isMap) {
					Log::out << "mBins[i].mIdx.size() != mBins[i].mMaps.size()" << Log::endl;
					Log::out << "mBins[i].mIdx.size()= " << mBins[i].mIdx.size() << Log::endl;
					Log::out << "mBins[i].mMaps.size()= " << mBins[i].mBits[IdxP].mMaps.size() << Log::endl;
				}

				else if (i < mBinCount[0] && mBins[i].mBits[IdxP].mPos.size() != mNumBits[0] && isPos)
				{
					Log::out << "mBins[i].mBits.mPos.size() != mNumBits" << Log::endl;

				}
				else if (i > mBinCount[0] - 1 && mBins[i].mBits[IdxP].mPos.size() != mNumBits[1] && isPos)
				{
					Log::out << "mBins[i].mBits.mPos.size() != mNumBits" << Log::endl;
				}
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

	void SimpleHasher1::init(u64 n)
	{	
		mN = n;
		mMaxBinSize[0] = 32;
		mMaxBinSize[1] = 64;

		mBinCount[0] = 1.15*n;
		mBinCount[1] = 0.17*n;

		mMtx.reset(new std::mutex[mBinCount[0] + mBinCount[1]]);
		mBins.resize(mBinCount[1] + mBinCount[0]);
		mNumHashes[0] = 3;
		mNumHashes[1] = 2;
		mNumBits[0] = 5;
		mNumBits[1] = 6;

	//	auto log2n = log2ceil(n);

//		mInputBitSize = numBits;

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
		

	}

	void SimpleHasher1::insertBatch(ArrayView<u64> inputIdxs, MatrixView<u64> hashs)
	{
		for (u64 j = 0; j < inputIdxs.size(); ++j)
		{
			for (u64 k = 0; k < mNumHashes[0]; ++k)
			{
				u64 addr = *(u64*)&hashs[j][k] % mBinCount[0];
				//if(addr==0)
				//	std::cout << "----"<<inputIdxs[j] <<"-" << addr << std::endl;

				std::lock_guard<std::mutex> lock(mMtx[addr]);
				if (std::find(mBins[addr].mIdx.begin(), mBins[addr].mIdx.end(), inputIdxs[j]) 
					== mBins[addr].mIdx.end()) {
					{	
							mBins[addr].mIdx.emplace_back(inputIdxs[j]);
					//		std::cout << "1----"<<inputIdxs[j] <<"-" << addr << std::endl;
					}
				}
			}

			for (u64 k = 0; k < mNumHashes[1]; ++k)
			{
				u64 addrStash = *(u64*)&hashs[j][k] % mBinCount[1] + mBinCount[0];

				std::lock_guard<std::mutex> lock(mMtx[addrStash]);
				if (std::find(mBins[addrStash].mIdx.begin(), mBins[addrStash].mIdx.end(), inputIdxs[j])
					== mBins[addrStash].mIdx.end()) {				
					mBins[addrStash].mIdx.emplace_back(inputIdxs[j]);
					//	std::cout << "2----" << inputIdxs[j] << "-" << addrStash << std::endl;

				}
			}

		}
	}
}

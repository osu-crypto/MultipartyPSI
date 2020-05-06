#include "SimpleHasher1.h"
#include "Crypto/sha1.h"
#include "Crypto/PRNG.h"
#include <random>
#include "Common/Log.h"
#include "Common/Log1.h"
#include <numeric>

namespace osuCrypto
{

	SimpleParam1 k2n24s40SimpleParam1
	{ { 1.11,0.17 },{ 3,2 },{ 31,63 },{5,6} };
	SimpleParam1 k2n20s40SimpleParam1
	{ { 1.12,0.17 },{ 3,2 },{ 30,63 } ,{ 5,6 } };
	SimpleParam1 k2n16s40SimpleParam1
	{ { 1.13,0.16 },{ 3,2 },{ 29,63 },{ 5,6 } };
	SimpleParam1 k2n14s40SimpleParam1
	{ { 1.14,0.16 },{ 3,2 },{ 28,63 },{ 5,6 } };
	SimpleParam1 k2n12s40SimpleParam1
	{ { 1.17,0.15 },{ 3,2 },{ 27,63 },{ 5,6 } };
	SimpleParam1 k2n08s40SimpleParam1
	{ { 1.17,0.15 },{ 3,2 },{ 27,63 },{ 5,6 } };

	// not sure if this needs a stash of 40, but should be safe enough.
	SimpleParam1 k2n07s40SimpleParam1
	{ { 1.5,0.17 },{ 3,2 },{ 27,64 },{ 5,6 } };


	SimpleHasher1::SimpleHasher1()
	{
	}


	SimpleHasher1::~SimpleHasher1()
	{
	}


	u64 SimpleHasher1::maxRealBinSize() {
	
		u64 rs=0;
		for (u64 i = 0; i < mBins.size(); ++i)
		{
			if (mBins[i].mIdx.size() > 0)
			{
				if (rs < mBins[i].mIdx.size())
					rs = mBins[i].mIdx.size();
			}
		}
		
		realBinSizeCount1.resize(32);
		for (u64 i = 0; i < mBinCount[0]; ++i)
		{
			for (u64 j = 0; j < realBinSizeCount1.size(); j++)
			{
				if (mBins[i].mIdx.size() == j)
					realBinSizeCount1[j]++;
			}
			
		}		

		realBinSizeCount2.resize(64);
		for (u64 i = mBinCount[0]; i < mBins.size(); ++i)
		{
			for (u64 j = 0; j < realBinSizeCount2.size(); j++)
			{
				if (mBins[i].mIdx.size() == j)
					realBinSizeCount2[j]++;
			}

		}

		return rs;
	}

	void SimpleHasher1::print(u64 IdxP, bool isIdx, bool isOPRF, 
		bool isMap, bool isPos, u64 opt) const
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
							
						std::cout << "    c_idx=" << mBins[i].mIdx[j] << "  hIdx=" << mBins[i].hIdx[j] << std::endl;


						if (isOPRF && (opt == 0 || opt==1))//seperated oprf
							Log::out << "    c_OPRF=" << mBins[i].mValOPRF[IdxP][j];
						if (isOPRF && (opt == 2 || opt == 3))//combined oprf
							Log::out <<"    mOprfs="
							<< mOprfs[IdxP][mBins[i].mIdx[j]][mBins[i].hIdx[j]];

						if (isMap && opt==0)
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

	void SimpleHasher1::init(u64 n,u64 opt)
	{	
		mN = n;

		if (n <= 1 << 7)
			mParams = k2n07s40SimpleParam1;
		else if (n <= 1 << 8)
			mParams = k2n08s40SimpleParam1;
		else if (n <= 1 << 12)
			mParams = k2n12s40SimpleParam1;
		else if (n <= 1 << 14)
			mParams = k2n14s40SimpleParam1;
		else if (n <= 1 << 16)
			mParams = k2n16s40SimpleParam1;
		else if (n <= 1 << 20)
			mParams = k2n20s40SimpleParam1;
		else if (n <= 1 << 24)
			mParams = k2n24s40SimpleParam1;
		else
			throw std::runtime_error("not implemented");

		if (opt == 0)
		{
			mParams.mMaxBinSize[0] = std::pow(2,std::ceil(std::log2(mParams.mMaxBinSize[0])));
			mParams.mMaxBinSize[1] = std::pow(2, std::ceil(std::log2(mParams.mMaxBinSize[1])));
		}


		mMaxBinSize[0] = mParams.mMaxBinSize[0];
		mMaxBinSize[1] = mParams.mMaxBinSize[1];

		mBinCount[0] = mParams.mBinScaler[0]*n;
		mBinCount[1] = mParams.mBinScaler[1] *n;

		mMtx.reset(new std::mutex[mBinCount[0] + mBinCount[1]]);
		mBins.resize(mBinCount[1] + mBinCount[0]);
		mNumHashes[0] = mParams.mNumHashes[0];
		mNumHashes[1] = mParams.mNumHashes[1];
		mNumBits[0] = mParams.mNumBits[0];
		mNumBits[1] = mParams.mNumBits[1];

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
							mBins[addr].hIdx.emplace_back(k);
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
					mBins[addrStash].hIdx.emplace_back(mNumHashes[0]+k);
					//	std::cout << "2----" << inputIdxs[j] << "-" << addrStash << std::endl;

				}
			}

		}
	}
}

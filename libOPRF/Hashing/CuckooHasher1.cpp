#include "CuckooHasher1.h"
#include "Crypto/sha1.h"
#include "Crypto/PRNG.h"
#include <random>
#include "Common/Log.h"
#include "Common/Log1.h"
#include <numeric>

namespace osuCrypto
{

    // parameters for k=2 hash functions, 2^n items, and statistical security 40
	//double mBinScaler;
	//u64 mNumHashes;
	//u64 mSenderBinSize;

	//double mBinStashScaler;
	//u64 mNumStashHashes;
	//u64 mSenderBinStashSize;

    CuckooParam1 k2n24s40CuckooParam1
	{ { 1.11,0.17 },{ 3,2 },{ 31,63 } };
    CuckooParam1 k2n20s40CuckooParam1
	{ { 1.12,0.17 },{ 3,2 },{ 30,63 } };
    CuckooParam1 k2n16s40CuckooParam1
	{ { 1.13,0.16 },{ 3,2 },{ 29,63 }};
    CuckooParam1 k2n14s40CuckooParam1
	{ { 1.14,0.16 },{ 3,2 },{ 28,63 } };
	CuckooParam1 k2n12s40CuckooParam1
	{ { 1.17,0.15 },{ 3,2 },{ 27,63 } };
    CuckooParam1 k2n08s40CuckooParam1
	{ { 1.17,0.15 },{ 3,2 },{ 27,63 } };

    // not sure if this needs a stash of 40, but should be safe enough.
    CuckooParam1 k2n07s40CuckooParam1
	{ { 1.5,0.17 },{ 3,2 },{ 27,64 } };


    CuckooHasher1::CuckooHasher1()
        :mTotalTries(0)
    {
    }

    CuckooHasher1::~CuckooHasher1()
    {
    }

    bool CuckooHasher1::operator==(const CuckooHasher1 & cmp) const
    {
        if (mBins.size() != cmp.mBins.size())
            throw std::runtime_error("");

        if (mStashBins.size() != cmp.mStashBins.size())
            throw std::runtime_error("");



        for (u64 i = 0; i < mBins.size(); ++i)
        {
            if (mBins[i].mVal != cmp.mBins[i].mVal)
            {
                return false;
            }
        }

        for (u64 i = 0; i < mStashBins.size(); ++i)
        {
            if (mStashBins[i].mVal != cmp.mStashBins[i].mVal)
            {
                return false;
            }
        }

        return true;
    }

    bool CuckooHasher1::operator!=(const CuckooHasher1 & cmp) const
    {
        return !(*this == cmp);
    }

    void CuckooHasher1::print(u64 IdxP, bool isIdx, bool isOPRF, bool isMap) const
    {
		std::cout << IoStream::lock;
		//std::cout << "Cuckoo Hasher  " << std::endl;
		Log::out << "Cuckoo Hasher  " << Log::endl;

		// for (u64 i = 0; i < 10; ++i)
        for (u64 i = 0; i < mBins.size(); ++i)
        {
			//Log::out << " contains 0 elements\n";
          //  std::cout << "Bin #" << i;
			Log::out << "Bin #" << i << Log::endl;

            if (mBins[i].isEmpty())
            {
            //    std::cout << " - " << std::endl;
				Log::out << " - " << Log::endl;

            }
            else
            {
              //  std::cout << "    c_idx=" << mBins[i].idx() << "  hIdx=" << mBins[i].hashIdx() << std::endl;
				if (isIdx)
				
					std::cout << "    c_idx=" << mBins[i].idx() << "  hIdx=" << mBins[i].hashIdx() << std::endl;


				if (isOPRF)
					Log::out << "    c_OPRF=" << mBins[i].mValOPRF[IdxP];
				
				if(isMap)
					Log::out << "    c_Map="<< static_cast<int16_t>(mBins[i].mValMap[IdxP]);
				
				Log::out << Log::endl;
            }

        }
	//	 for (u64 i = 0; i < 0 && mStash[i].isEmpty() == false; ++i)
			 for (u64 i = 0; i < mStashBins.size() && mStashBins[i].isEmpty() == false; ++i)
				 //for (u64 i = 0; i < mStashBins.size() ; ++i)
        {
            //std::cout << "Bin #" << i;
			Log::out << "SBin #" << i;

            if (mStashBins[i].isEmpty())
            {
              //  std::cout << " - " << std::endl;
				Log::out << " - " << Log::endl;

            }
            else
            {
			//	std::cout << "    c_idx=" << mStash[i].idx() << "  hIdx=" << mStash[i].hashIdx() << std::endl;
				Log::out << "    c_idx=" << mStashBins[i].idx() << "  hIdx=" << mStashBins[i].hashIdx() << Log::endl;

            }

        }
		
		//std::cout << std::endl;
		Log::out << Log::endl;
		std::cout << IoStream::unlock;

    }

    void CuckooHasher1::init(u64 n,u64 opt)
    {

       // if (statSecParam != 40) throw std::runtime_error("not implemented");

		
			if (n <= 1 << 7)
				mParams = k2n07s40CuckooParam1;			
			else if (n <= 1 << 8)
				mParams = k2n08s40CuckooParam1;
			else if (n <= 1 << 12)
				mParams = k2n12s40CuckooParam1;
			else if (n <= 1 << 14)
				mParams = k2n14s40CuckooParam1;
			else if (n <= 1 << 16)
				mParams = k2n16s40CuckooParam1;
			else if (n <= 1 << 20)
				mParams = k2n20s40CuckooParam1;
			else if (n <= 1 << 24)
				mParams = k2n24s40CuckooParam1;
			else
				throw std::runtime_error("not implemented");
		
			if (opt == 0)
			{
				mParams.mSenderBinSize[0] = std::pow(2, std::ceil(std::log2(mParams.mSenderBinSize[0])));;
				mParams.mSenderBinSize[1] = std::pow(2, std::ceil(std::log2(mParams.mSenderBinSize[1])));;
			}

        mHashes.resize(n * mParams.mNumHashes[0], u64(-1));
        mHashesView = MatrixView<u64>(mHashes.begin(), mHashes.end(), mParams.mNumHashes[0]);

		mStashHashes.resize(n * mParams.mNumHashes[1], u64(-1));
		mStashHashesView = MatrixView<u64>(mStashHashes.begin(), mStashHashes.end(), mParams.mNumHashes[1]);

		mBinCount[0] = (mParams.mBinScaler[0] ) * n;
		mBinCount[1] = ( mParams.mBinScaler[1]) * n;
		
			mBins.resize(mBinCount[0]+ mBinCount[1]);
			
    }

    void CuckooHasher1::insert(u64 inputIdx, ArrayView<u64> hashs)
    {
        if (mHashesView[inputIdx][0] != -1)
        {
            throw std::runtime_error("");
        }

        memcpy(mHashesView[inputIdx].data(), hashs.data(), sizeof(u64) * mParams.mNumHashes[0]);

        insertHelper(inputIdx, 0, 0);
    }

    void CuckooHasher1::insertBatch(
        ArrayView<u64> inputIdxs,
        MatrixView<u64> hashs,
        Workspace& w)
    {
        u64 width = mHashesView.size()[1];
        u64 remaining = inputIdxs.size();
        u64 tryCount = 0;

#ifndef  NDEBUG
        if (hashs.size()[1] != width)
            throw std::runtime_error("" LOCATION);
#endif // ! NDEBUG


        for (u64 i = 0; i < inputIdxs.size(); ++i)
        {
            for (u64 j = 0; j < mParams.mNumHashes[0]; ++j)
            {
                //mHashesView[inputIdxs[i]][j] = hashs[i][j];
                (mHashesView.data() + inputIdxs[i] * width)[j] = (hashs.data() + i * width)[j];
            }
            w.curHashIdxs[i] = 0;
        }


        while (remaining && tryCount++ < 100)
        {

            // this data fetch can be slow (after the first loop). 
            // As such, lets do several fetches in parallel.
            for (u64 i = 0; i < remaining; ++i)
            {
                //w.curAddrs[i] = mHashesView[inputIdxs[i]][w.curHashIdxs[i]] % mBins.size();
                w.curAddrs[i] = (mHashesView.data() + inputIdxs[i] * width)[w.curHashIdxs[i]] % mBinCount[0];
            }

            // same thing here, this fetch is slow. Do them in parallel.
            for (u64 i = 0; i < remaining; ++i)
            {
                u64 newVal = inputIdxs[i] | (w.curHashIdxs[i] << 56);
#ifdef THREAD_SAFE_CUCKOO
                w.oldVals[i] = mBins[w.curAddrs[i]].mVal.exchange(newVal, std::memory_order_relaxed);
#else
                w.oldVals[i] = mBins[w.curAddrs[i]].mVal;
                mBins[w.curAddrs[i]].mVal = newVal;
#endif
            }

            // this loop will update the items that were just evicted. The main
            // idea of that our array looks like
            //     |XW__Y____Z __|
            // For X and W, which failed to be placed, lets write over them
            // with the vaues that they evicted.
            u64 putIdx = 0, getIdx = 0;
            while (putIdx < remaining && w.oldVals[putIdx] != u64(-1))
            {
                inputIdxs[putIdx] = w.oldVals[putIdx] & (u64(-1) >> 8);
                w.curHashIdxs[putIdx] = (1 + (w.oldVals[putIdx] >> 56)) % mParams.mNumHashes[0];
                ++putIdx;
            }

            getIdx = putIdx + 1;

            // Now we want an array that looks like 
            //  |ABCD___________| but currently have 
            //  |AB__Y_____Z____| so lets move them 
            // forward and replace Y, Z with the values
            // they evicted.
            while (getIdx < remaining)
            {
                while (getIdx < remaining &&
                    w.oldVals[getIdx] == u64(-1))
                    ++getIdx;

                if (getIdx >= remaining) break;

                inputIdxs[putIdx] = w.oldVals[getIdx] & (u64(-1) >> 8);
                w.curHashIdxs[putIdx] = (1 + (w.oldVals[getIdx] >> 56)) % mParams.mNumHashes[0];

                // not needed. debug only
                std::swap(w.oldVals[putIdx], w.oldVals[getIdx]);

                ++putIdx;
                ++getIdx;
            }

            remaining = putIdx;
        }

		
		/*	ArrayView<u64> stashIdxs(inputIdxs.begin(), inputIdxs.begin() + remaining, false);
			MatrixView<u64> stashHashs(hashs.data(), remaining, mParams.mNumHashes, false);
			CuckooHasher1::Workspace stashW(remaining);
			std::vector<Bin> mStashBins;*/
			//mStashBins.insertBatch(stashIdxs, stashHashs, stashW, false);
		std::lock_guard<std::mutex> lock(mInsertBin);
			for (u64 i = 0; i < remaining; ++i)
			{
				mStashIdxs.push_back(inputIdxs[i]);
			}
		
		
	/*		for (u64 i = 0, j = 0; i < remaining; ++j)
			{
				mStashBins[j].swap(inputIdxs[i], w.curHashIdxs[i]);


				MatrixView<u64> hashs2;

				if (inputIdxs[i] == -1)
					++i;
			}*/
		

    }

	void CuckooHasher1::insertStashBatch(
		ArrayView<u64> inputIdxs,
		MatrixView<u64> hashs,
		Workspace& w)
	{

		u64 width = mStashHashesView.size()[1];

		u64 remaining = inputIdxs.size();
	//	std::cout << "inputStashIdxs.size(): " << inputIdxs.size() << std::endl;

		u64 tryCount = 0;

#ifndef  NDEBUG
		if (hashs.size()[1] != width)
			throw std::runtime_error("" LOCATION);
#endif // ! NDEBUG


		for (u64 i = 0; i < inputIdxs.size(); ++i)
		{
			for (u64 j = 0; j < mParams.mNumHashes[1]; ++j)
			{
				//mHashesView[inputIdxs[i]][j] = hashs[i][j];
				(mStashHashesView.data() + inputIdxs[i] * width)[j] = (hashs.data() + i * width)[j];
			}
			w.curHashIdxs[i] = 0;
		}


		while (remaining && tryCount++ < 100)
		{

			// this data fetch can be slow (after the first loop). 
			// As such, lets do several fetches in parallel.
			for (u64 i = 0; i < remaining; ++i)
			{
				//w.curAddrs[i] = mHashesView[inputIdxs[i]][w.curHashIdxs[i]] % mBins.size();
				w.curAddrs[i] = (mStashHashesView.data() + inputIdxs[i] * width)[w.curHashIdxs[i]] % mBinCount[1] + mBinCount[0];
			}

			// same thing here, this fetch is slow. Do them in parallel.
			for (u64 i = 0; i < remaining; ++i)
			{
				u64 newVal = inputIdxs[i] | (w.curHashIdxs[i] << 56);
#ifdef THREAD_SAFE_CUCKOO
				w.oldVals[i] = mBins[w.curAddrs[i]].mVal.exchange(newVal, std::memory_order_relaxed);
#else
				w.oldVals[i] = mBins[w.curAddrs[i]].mVal;
				mBins[w.curAddrs[i]].mVal = newVal;
#endif
			}

			// this loop will update the items that were just evicted. The main
			// idea of that our array looks like
			//     |XW__Y____Z __|
			// For X and W, which failed to be placed, lets write over them
			// with the vaues that they evicted.
			u64 putIdx = 0, getIdx = 0;
			while (putIdx < remaining && w.oldVals[putIdx] != u64(-1))
			{
				inputIdxs[putIdx] = w.oldVals[putIdx] & (u64(-1) >> 8);
				w.curHashIdxs[putIdx] = (1 + (w.oldVals[putIdx] >> 56)) % mParams.mNumHashes[1];
				++putIdx;
			}

			getIdx = putIdx + 1;

			// Now we want an array that looks like 
			//  |ABCD___________| but currently have 
			//  |AB__Y_____Z____| so lets move them 
			// forward and replace Y, Z with the values
			// they evicted.
			while (getIdx < remaining)
			{
				while (getIdx < remaining &&
					w.oldVals[getIdx] == u64(-1))
					++getIdx;

				if (getIdx >= remaining) break;

				inputIdxs[putIdx] = w.oldVals[getIdx] & (u64(-1) >> 8);
				w.curHashIdxs[putIdx] = (1 + (w.oldVals[getIdx] >> 56)) % mParams.mNumHashes[1];

				// not needed. debug only
				std::swap(w.oldVals[putIdx], w.oldVals[getIdx]);

				++putIdx;
				++getIdx;
			}

			remaining = putIdx;
		}

		if (remaining > 0)
		{
			std::cout << "remaining: " << remaining << std::endl;
			throw std::runtime_error("" LOCATION);
		}
	}


    void CuckooHasher1::insertHelper(u64 inputIdx, u64 hashIdx, u64 numTries)
    {
        //++mTotalTries;

        u64 xrHashVal = mHashesView[inputIdx][hashIdx];

        auto addr = (xrHashVal) % mBins.size();

        // replaces whatever was in this bin with our new item
        //mBins[addr].swap(inputIdx, hashIdx);
        {

            u64 newVal = inputIdx | (hashIdx << 56);
#ifdef THREAD_SAFE_CUCKOO
            u64 oldVal = mBins[addr].mVal.exchange(newVal, std::memory_order_relaxed);
#else
            u64 oldVal = mBins[addr].mVal;
            mBins[addr].mVal = newVal;
#endif

            if (oldVal == u64(-1))
            {
                inputIdx = u64(-1);
            }
            else
            {
                inputIdx = oldVal & (u64(-1) >> 8);
                hashIdx = oldVal >> 56;
            }
        }

        if (inputIdx != -1)
        {

            // if idxItem is anything but -1, then we just exicted something. 
            if (numTries < 100)
            {
                // lets try to insert it into its next location
                insertHelper(inputIdx, (hashIdx + 1) % mParams.mNumHashes[0], numTries + 1);
            }
            else
            {
                // put in stash
                for (u64 i = 0; inputIdx != u64(-1); ++i)
                {
                    mStashBins[i].swap(inputIdx, hashIdx);
                }

            }
        }

    }

	void CuckooHasher1::insertStashHelper(u64 inputIdx, u64 hashIdx, u64 numTries)
	{
		//++mTotalTries;

		u64 xrHashVal = mHashesView[inputIdx][hashIdx];

		auto addr = (xrHashVal) % mStashBins.size();

		// replaces whatever was in this bin with our new item
		//mBins[addr].swap(inputIdx, hashIdx);
		{

			u64 newVal = inputIdx | (hashIdx << 56);
#ifdef THREAD_SAFE_CUCKOO
			u64 oldVal = mBins[addr].mVal.exchange(newVal, std::memory_order_relaxed);
#else
			u64 oldVal = mStashBins[addr].mVal;
			mStashBins[addr].mVal = newVal;
#endif

			if (oldVal == u64(-1))
			{
				inputIdx = u64(-1);
			}
			else
			{
				inputIdx = oldVal & (u64(-1) >> 8);
				hashIdx = oldVal >> 56;
			}
		}

		if (inputIdx != -1)
		{

			// if idxItem is anything but -1, then we just exicted something. 
			if (numTries < 100)
			{
				// lets try to insert it into its next location
				insertStashHelper(inputIdx, (hashIdx + 1) % mParams.mNumHashes[0], numTries + 1);
			}
			else
			{
				// put in stash
				for (u64 i = 0; inputIdx != u64(-1); ++i)
				{
					mStashBins[i].swap(inputIdx, hashIdx);
				}

			}
		}

	}


#if 0

    u64 CuckooHasher1::find(ArrayView<u64> hashes)
    {
        if (mParams.mNumHashes[0] == 2)
        {
            std::array<u64, 2>  addr{
                (hashes[0]) % mBins.size(),
                (hashes[1]) % mBins.size() };

#ifdef THREAD_SAFE_CUCKOO
            std::array<u64, 2> val{
                mBins[addr[0]].mVal.load(std::memory_order::memory_order_relaxed),
                mBins[addr[1]].mVal.load(std::memory_order::memory_order_relaxed) };
#else
            std::array<u64, 2> val{
                mBins[addr[0]].mVal,
                mBins[addr[1]].mVal };
#endif

            if (val[0] != -1)
            {
                u64 itemIdx = val[0] & (u64(-1) >> 8);

                bool match =
                    (mHashesView[itemIdx][0] == hashes[0]) &&
                    (mHashesView[itemIdx][1] == hashes[1]);

                if (match) return itemIdx;
            }

            if (val[1] != -1)
            {
                u64 itemIdx = val[1] & (u64(-1) >> 8);

                bool match =
                    (mHashesView[itemIdx][0] == hashes[0]) &&
                    (mHashesView[itemIdx][1] == hashes[1]);

                if (match) return itemIdx;
            }


            // stash

            i64 i = 0;
            while (i < mStashBins.size() && mStashBins[i].isEmpty() == false)
            {
#ifdef THREAD_SAFE_CUCKOO
                u64 val = mStash[i].mVal.load(std::memory_order::memory_order_relaxed);
#else
                u64 val = mStashBins[i].mVal;
#endif
                if (val != u64(-1))
                {
                    u64 itemIdx = val & (u64(-1) >> 8);

                    bool match =
                        (mHashesView[itemIdx][0] == hashes[0]) &&
                        (mHashesView[itemIdx][1] == hashes[1]);

                    if (match)
                    {
                        return itemIdx;
                    }
                }

                ++i;
            }

        }
        else
        {

            for (u64 i = 0; i < mParams.mNumHashes[0]; ++i)
            {
                u64 xrHashVal = hashes[i];
                auto addr = (xrHashVal) % mBins.size();


#ifdef THREAD_SAFE_CUCKOO
                u64 val = mBins[addr].mVal.load(std::memory_order::memory_order_relaxed);
#else
                u64 val = mBins[addr].mVal;
#endif

                if (val != u64(-1))
                {
                    u64 itemIdx = val & (u64(-1) >> 8);

                    bool match = true;
                    for (u64 j = 0; j < mParams.mNumHashes; ++j)
                    {
                        match &= (mHashesView[itemIdx][j] == hashes[j]);
                    }

                    if (match)
                    {
                        return itemIdx;
                    }
                }
            }

            i64 i = 0;
            while (i < mStashBins.size() && mStashBins[i].isEmpty() == false)
            {
#ifdef THREAD_SAFE_CUCKOO
                u64 val = mStash[i].mVal.load(std::memory_order::memory_order_relaxed);
#else
                u64 val = mStashBins[i].mVal;
#endif

                if (val != u64(-1))
                {
                    u64 itemIdx = val & (u64(-1) >> 8);

                    bool match = true;
                    for (u64 j = 0; j < mParams.mNumHashes[0]; ++j)
                    {
                        match &= (mHashesView[itemIdx][j] == hashes[j]);
                    }

                    if (match)
                    {
                        return itemIdx;
                    }
                }

                ++i;
            }
        }
        //}

        return u64(-1);
    }


    u64 CuckooHasher1::findBatch(
        MatrixView<u64> hashes,
        ArrayView<u64> idxs,
        Workspace& w)
    {

        if (mParams.mNumHashes[0] == 2)
        {
            std::array<u64, 2>  addr;

            for (u64 i = 0; i < hashes.size()[0]; ++i)
            {
                idxs[i] = -1;

                addr[0] = (hashes[i][0]) % mBins.size();
                addr[1] = (hashes[i][1]) % mBins.size();

#ifdef THREAD_SAFE_CUCKOO
                w.findVal[i][0] = mBins[addr[0]].mVal.load(std::memory_order::memory_order_relaxed);
                w.findVal[i][1] = mBins[addr[1]].mVal.load(std::memory_order::memory_order_relaxed);
#else
                w.findVal[i][0] = mBins[addr[0]].mVal;
                w.findVal[i][1] = mBins[addr[1]].mVal;
#endif
            }

            for (u64 i = 0; i < hashes.size()[0]; ++i)
            {
                if (w.findVal[i][0] != -1)
                {
                    u64 itemIdx = w.findVal[i][0] & (u64(-1) >> 8);

                    bool match =
                        (mHashesView[itemIdx][0] == hashes[i][0]) &&
                        (mHashesView[itemIdx][1] == hashes[i][1]);

                    if (match) idxs[i] = itemIdx;
                }

                if (w.findVal[i][1] != -1)
                {
                    u64 itemIdx = w.findVal[i][1] & (u64(-1) >> 8);

                    bool match =
                        (mHashesView[itemIdx][0] == hashes[i][0]) &&
                        (mHashesView[itemIdx][1] == hashes[i][1]);

                    if (match) idxs[i] = itemIdx;
                }
            }

            // stash

            i64 i = 0;
            while (i < mStashBins.size() && mStashBins[i].isEmpty() == false)
            {
#ifdef THREAD_SAFE_CUCKOO
                u64 val = mStash[i].mVal.load(std::memory_order::memory_order_relaxed);
#else
                u64 val = mStashBins[i].mVal;
#endif
                if (val != u64(-1))
                {
                    u64 itemIdx = val & (u64(-1) >> 8);

                    for (u64 j = 0; j < hashes.size()[0]; ++j)
                    {

                        bool match =
                            (mHashesView[itemIdx][0] == hashes[j][0]) &&
                            (mHashesView[itemIdx][1] == hashes[j][1]);

                        if (match)
                        {
                            idxs[j] = itemIdx;
                        }
                    }
                }

                ++i;
            }

        }
        else
        {
            throw std::runtime_error("not implemented");
        }
        return u64(-1);
    }

#endif


    bool CuckooHasher1::Bin::isEmpty() const
    {
        return mVal == u64(-1);
    }

    u64 CuckooHasher1::Bin::idx() const
    {
        return mVal  & (u64(-1) >> 8);
    }

    u64 CuckooHasher1::Bin::hashIdx() const
    {
        return mVal >> 56;
    }

    void CuckooHasher1::Bin::swap(u64 & idx, u64 & hashIdx)
    {
        u64 newVal = idx | (hashIdx << 56);
#ifdef THREAD_SAFE_CUCKOO
        u64 oldVal = mVal.exchange(newVal, std::memory_order_relaxed);
#else
        u64 oldVal = mVal;
        mVal = newVal;
#endif
        if (oldVal == u64(-1))
        {
            idx = hashIdx = u64(-1);
        }
        else
        {
            idx = oldVal & (u64(-1) >> 8);
            hashIdx = oldVal >> 56;
        }
    }
}

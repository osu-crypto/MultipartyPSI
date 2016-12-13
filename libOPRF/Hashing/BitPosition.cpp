#include "BitPosition.h"
#include "Crypto/sha1.h"
#include "Crypto/PRNG.h"
#include <random>
#include "Common/Log.h"
#include "Common/Log1.h"
#include <numeric>


namespace osuCrypto
{
	int isSet(block & v, int n)
	{
		__m128i chkmask = _mm_slli_epi16(_mm_set1_epi16(1), n & 0xF);
		int     movemask = (1 << (n >> 3));
		int     isSet = (((_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_and_si128(chkmask, v), _mm_setzero_si128())) & movemask) ^ movemask));
		return isSet;
	}
	void setBit(block & v, int pos)
	{
		__m128i shuf = _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
		shuf = _mm_add_epi8(shuf, _mm_set1_epi8(16 - (pos >> 3)));
		shuf = _mm_and_si128(shuf, _mm_set1_epi8(0x0F));
		__m128i setmask = _mm_shuffle_epi8(_mm_cvtsi32_si128(1 << (pos & 0x7)), shuf);
		v = _mm_or_si128(v, setmask);
	}
	bool TestBitN(__m128i value, int N)
	{
		__m128i positioned = _mm_slli_epi64(value, 7 - (N & 7));
		return (_mm_movemask_epi8(positioned) & (1 << (N / 8))) != 0;
	}

	int midIdx(std::vector<block>& codewords, int length, std::vector<u8>& rs)
	{
		int temp = 0;
		int idx = 0;
		int mid = 0;
		int cnt = 0;
		//std::cout << "temp ";
		for (size_t j = 0; j < length; j++)
		{
			temp = 0;
			if (std::find(rs.begin(), rs.end(),  j) == rs.end())
			{
				cnt++;
				for (size_t i = 0; i < codewords.size(); i++)
				{
					if (TestBitN(codewords[i], j))
						temp++;
				}
				//std::cout << j << "," << temp << " ";
				if (temp < codewords.size() / 2 && mid < temp)
				{
					mid = temp;
					idx = j;
				}
			}
		}
	//	std::cout << idx << " - " << mid << " - " << codewords.size() << " - " << cnt << std::endl;
		return idx;
	}


	BitPosition::BitPosition()
    {
    }

	BitPosition::~BitPosition()
    {
    }

   
    void BitPosition::print() const
    {
		std::cout << "mPos: \n";
		for (auto it = mPos.begin(); it != mPos.end(); ++it)
		{
				std::cout << static_cast<int16_t>(*it) << "  ";
		}
		std::cout << std::endl;
		std::cout << "Masks: ";
		for (auto iter = mMaps.begin(); iter != mMaps.end(); ++iter) {
			std::cout << static_cast<int16_t>((*iter)) << " ";
		}

    }
	void BitPosition::init(u64 numRealCodeWord, u64 numMaxBitSize)
    {
		mRealBitSize= std::floor(std::log2(numRealCodeWord)) + 1;
		mMaxBitSize = numMaxBitSize;

    }
	

	void BitPosition::getMasks(std::vector<block>& codeword) {
		
		u8 rs, idx;
		for (int i = 0; i < codeword.size(); i++) {
			rs = 0;
			idx = 1;
			for (auto it = mPos.begin(); it != mPos.end(); ++it)
			{
				if (TestBitN(codeword[i], *it))
				{
					rs = rs^idx;
					//std::cout << static_cast<int16_t>(idx) << std::endl;
				}
				idx = idx << 1;
			}
			mMaps.push_back(rs);
		}
	}
	void BitPosition::getMask(block& codeword, u8& mask) {

		u8 rs, idx;		
		mask = 0;
			idx = 1;
			for (auto it = mPos.begin(); it != mPos.end(); ++it)
			{
				if (TestBitN(codeword, *it))
				{
					mask = mask^idx;
					//std::cout << static_cast<int16_t>(idx) << std::endl;
				}
				idx = idx << 1;
			}		
	}
	
	void BitPosition::getPosHelper(std::vector<block>& codewords, int length)
	{
		int setSize = codewords.size();
		std::vector<block> testSet1;
		std::vector<block> testSet2;
		
		if (mPos.size() < mRealBitSize){
			int idx = midIdx(codewords, length, mPos);

			if (std::find(mPos.begin(), mPos.end(), idx) == mPos.end())
			{
				mPos.push_back(idx);
					for (size_t i = 0; i < setSize; i++)

						if (TestBitN(codewords[i], idx))
						{
							//std::cout << "TestBitN=1: " << i << std::endl;
							testSet1.push_back(codewords[i]);
						}
						else
						{
							//std::cout << "TestBitN=0: " << i << std::endl;
							testSet2.push_back(codewords[i]);
						}
					//std::cout <<"testSet1 " <<  testSet1.size() << std::endl;
					//std::cout <<"testSet2 "<< testSet2.size() << std::endl;
					getPos(testSet1, length);
					getPos(testSet2, length);
				}
		}
	}

	void BitPosition::getPos(std::vector<block>& codewords, int length)
	{
		getPosHelper(codewords, length);
										 		while(mPos.size()<mMaxBitSize)
										 		{
										 			u64 rand = std::rand() % 128; //choose randome bit location
													if (std::find(mPos.begin(), mPos.end(), rand) == mPos.end())
													{
														mPos.push_back(rand);
													}
										 		}

									 
	}
	
}

//void BitPosition::findPos(std::vector<block>& codewords) {
//	bool isDone=false;
//	mMasks.clear();
//	std::set<u8>::iterator it;
//	std::pair<std::set<u8>::iterator, bool> ret;
//	std::pair<std::set<u8>::iterator, bool> retMask;

//	mNumTrial = 0;
//	while (!isDone) {
//		mPos.clear();
//		mMasks.clear();
//		for (u8 i = 0; i < mSize; i++)
//		{
//			u64 rand = std::rand() % 128; //choose randome bit location
//			ret = mPos.insert(rand);
//			if (ret.second == false) 
//				it = ret.first;
//		}
//	//	std::cout << "\n\n ";
//		//print();
//		isDone = true;	
//		//std::set<u8> masks1;

//		
//		for (u8 i = 0; i < codewords.size(); i++)
//		{
//			auto m = map(codewords[i]);
//			std::cout << static_cast<int16_t>(m) << " ";
//			retMask = mMasks.insert(m);
//			if (retMask.second == false) {
//				//std::cout << mNumTrial << std::endl;
//				isDone = false;
//				mNumTrial++;
//				break;
//			}
//		}
//		std::cout <<"\n-------\n";
//	}
//	//std::cout << static_cast<int16_t>(masks[0]) << " ";
//}

//u8 BitPosition::map(block& codeword) {
//	u8 rs = 0;
//	u8 idx = 0;
//	for (auto it = mPos.begin(); it != mPos.end(); ++it)
//	{
//		//	int i = *it / 8; //index of a block of 8 bits
//		//	int r = *it % 8;		//get bit location in i
//
//		////	std::cout << " " << i << ":" << r << std::endl;
//		////	__int8 c = codeword.m128i_i8[i];  //a block of 8 bits
//
//		//	//std::cout << static_cast<int16_t>(c) << std::endl;
//		//	u8 cq = ((codeword.m128i_i8[i] << (7 - r))); //shift to rightmost and left most to get only the single bit
//		//	cq = (cq >> 7) << idx; //then shift to location r
//		//	//std::cout << static_cast<int16_t>(cq) << std::endl;
//		//	rs = rs ^ cq; 
//		//	idx++;
//		//std::cout << std::endl;
//	}
//	//	std::cout << "rs: " << static_cast<int16_t>(rs) << std::endl;
//	return rs;
//}

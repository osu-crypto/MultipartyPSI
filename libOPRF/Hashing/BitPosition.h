#pragma once
#include "Common/Defines.h"
#include "Common/Log.h"
#include "Common/BitVector.h"
#include "Common/ArrayView.h"
#include "Common/MatrixView.h"
//#include <mutex>
#include <atomic>
#include <unordered_map>
//#define THREAD_SAFE_CUCKOO
#include <set>

namespace osuCrypto
{
 
    class BaseOPPRF
    {
    public:
		BaseOPPRF();
        ~BaseOPPRF();

    
        u64 mRealBitSize, mMaxBitSize, mNumTrial;
		std::vector<u8> mPos; //key: bit location; value: index of 
		std::vector<u8> mMaps;

		void print() const;
		void init(/*u64 numRealCodeWord,*/ u64 numMaxBitSize);
		bool getMasks(std::vector<block>& codeword);
		void getMask(block& codeword,u8& mask);
		void getPosHelper(std::vector<block>& codewords, int length);
		void getPos(std::vector<block>& codewords, int length);
		void getPos1(std::vector<block>& codewords, int length);
		void getRandPos();
		int midIdx(std::vector<block>& codewords, int length);

		//void findPos(std::vector<block>& codewords);
		//int isSet(block& codeword, int pos);
		//void setBit(block& codeword, int pos);
		//bool TestBitN(__m128i value, int N);
    };

}

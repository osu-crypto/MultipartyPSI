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
 
    class BitPosition
    {
    public:
		BitPosition();
        ~BitPosition();

    
        u64 mSize, mNumTrial;
		std::set<u8> mPos; //key: bit location; value: index of 
		std::vector<u8> mMasks;

		void print() const;
		void init(u64 n);
        void findPos(std::vector<block>& codewords, std::set<u8>& masks);
		u8 map(block& codeword);
/*		void findPos(std::vector<uint64_t>& codewords, std::vector<u8>& masks);
		u8 map(uint64_t& codeword);
		void findPos(std::vector<int32_t>& codewords, std::vector<u8>& masks);
		u8 map(int32_t& codeword*/

		int isSet(block& codeword, int pos);
		void setBit(block& codeword, int pos);
		bool TestBitN(__m128i value, int N);
		int midIdx(std::vector<block>& codewords, int length, std::set<int>& rs);
		void getIdxs(std::vector<block>& codewords, int length, std::set<int>& rs, int size);
    };

}

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

namespace osuCrypto
{
 
    class BitPosition
    {
    public:
		BitPosition();
        ~BitPosition();

    
        u64 mSize, mNumTrial;
		std::unordered_map<u8,u8> mPos; //key: bit location; value: index of 
		std::vector<u8> mMasks;

		void print() const;
		void init(u64 n);
        void findPos(std::vector<block>& codewords, std::vector<u8>& masks);
		u8 map(block& codeword);
/*		void findPos(std::vector<uint64_t>& codewords, std::vector<u8>& masks);
		u8 map(uint64_t& codeword);
		void findPos(std::vector<int32_t>& codewords, std::vector<u8>& masks);
		u8 map(int32_t& codeword*/
    };

}

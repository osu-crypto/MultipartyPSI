#include "BitPosition.h"
#include "Crypto/sha1.h"
#include "Crypto/PRNG.h"
#include <random>
#include "Common/Log.h"
#include "Common/Log1.h"
#include <numeric>


namespace osuCrypto
{
	
	BitPosition::BitPosition()
    {
    }

	BitPosition::~BitPosition()
    {
    }

   
    void BitPosition::print() const
    {
		std::cout << "loc: ";
		for (auto it = mPos.begin(); it != mPos.end(); ++it)
		{
				std::cout << static_cast<int16_t>(*it) << "  ";
		}
		std::cout << std::endl;
    }
    void BitPosition::init(u64 n)
    {
		mSize = std::floor(std::log2(n))+1;
    }
	void BitPosition::findPos(std::vector<block>& codewords, std::set<u8>& masks) {
		bool isDone=false;
		masks.clear();
		std::set<u8>::iterator it;
		std::pair<std::set<u8>::iterator, bool> ret;
		

		mNumTrial = 0;
		while (!isDone) {
			mPos.clear();
			masks.clear();
			for (u8 i = 0; i < mSize; i++)
			{
				u64 rand = std::rand() % 128; //choose randome bit location
				ret = mPos.insert(rand);
				if (ret.second == false) 
					it = ret.first;
			}
		//	std::cout << "\n\n ";
			//print();
			isDone = true;	
			std::set<u8> masks1;

			std::pair<std::set<u8>::iterator, bool> retMask;
			for (u8 i = 0; i < codewords.size(); i++)
			{
				auto m = map(codewords[i]);
				//std::cout << static_cast<int16_t>(m) << " ";
				retMask = masks1.insert(m);
				if (retMask.second == false) {
					//std::cout << mNumTrial << std::endl;
					isDone = false;
					mNumTrial++;
					break;
				}
			}
			//std::cout <<"\n-------\n";
		}
		//std::cout << static_cast<int16_t>(masks[0]) << " ";
	}


	u8 BitPosition::map(block& codeword) {
		u8 rs = 0;
		u8 idx = 0;
		for (auto it = mPos.begin(); it != mPos.end(); ++it)
		{
			int i = *it / 8; //index of a block of 8 bits
			int r = *it % 8;		//get bit location in i

		//	std::cout << " " << i << ":" << r << std::endl;
		//	__int8 c = codeword.m128i_i8[i];  //a block of 8 bits

			//std::cout << static_cast<int16_t>(c) << std::endl;
			u8 cq = ((codeword.m128i_i8[i] << (7 - r))); //shift to rightmost and left most to get only the single bit
			cq = (cq >> 7) << idx; //then shift to location r
			//std::cout << static_cast<int16_t>(cq) << std::endl;
			rs = rs ^ cq; 
			idx++;
			//std::cout << std::endl;
		}
		//std::cout << "rs: " << static_cast<int16_t>(rs) << std::endl;
		return rs;
	}

#if 0
	u8 BitPosition::map(block& codeword) {
		u8 rs = 0;
		for (auto it = mPos.begin(); it != mPos.end(); ++it)
		{
			int i = it->first / 8; //index of a block of 8 bits
			int r = it->first % 8;		//get bit location in i

										//	std::cout << " " << i << ":" << r << std::endl;
			//__int8 c = codeword[i]  //a block of 8 bits
								  //std::cout << static_cast<int16_t>(c) << std::endl;
				u8 cq = ((codeword.m128i_i8[i] << (7 - r))); //shift to rightmost and left most to get only the single bit
			cq = (cq >> 7) << it->second; //then shift to location r
										  //std::cout << static_cast<int16_t>(cq) << std::endl;
			rs = rs | cq;
			//std::cout << std::endl;
		}
		//std::cout << "rs: " << static_cast<int16_t>(rs) << std::endl;
		return rs;
	}

	void BitPosition::findPos(std::vector<int32_t>& codewords, std::vector<u8>& masks) {
		bool isDone = false;
		std::unordered_map<u8, u8> temp;
		masks.resize(codewords.size());
		mNumTrial = 0;
		while (!isDone) {
			temp.clear();
			mPos.clear();
			masks.clear();
			for (u8 i = 0; i < mSize; i++)
			{
				u64 rand = std::rand() % 128; //choose randome bit location
											  //std::cout << " | "<< rand << " ";
				if (mPos.find(rand) == mPos.end()) //check if not exists
					mPos.emplace(rand, i);
			}
			//	std::cout << "\n\n ";
			//print();
			isDone = true;
#if 1			

			//std::cout << "rs: ";
			for (u8 i = 0; i < codewords.size(); i++)
			{

				auto m = map(codewords[i]);
				if (temp.find(m) == temp.end())
				{
					temp[m];
					masks.push_back(m);
					//std::cout <<static_cast<int16_t>(m) <<" ";
				}
				else
				{
					//std::cout << " | "<<static_cast<int16_t>(m) << " ";
					std::cout << mNumTrial << std::endl;
					isDone = false;
					mNumTrial++;
					break;
				}
			}
			//	std::cout << std::endl;
#endif
		}
		//std::cout << static_cast<int16_t>(masks[0]) << " ";

	}
	u8 BitPosition::map(int32_t& codeword) {
		u8 rs = 0;
		for (auto it = mPos.begin(); it != mPos.end(); ++it)
		{
			int i = it->first / 8; //index of a block of 8 bits
			int r = it->first % 8;		//get bit location in i

										//	std::cout << " " << i << ":" << r << std::endl;
			__int8 c = codeword.m128[i];  //a block of 8 bits

											  //std::cout << static_cast<int16_t>(c) << std::endl;
			u8 cq = ((c << (7 - r))); //shift to rightmost and left most to get only the single bit
			cq = (cq >> 7) << it->second; //then shift to location r
										  //std::cout << static_cast<int16_t>(cq) << std::endl;
			rs = rs | cq;
			//std::cout << std::endl;
		}
		//std::cout << "rs: " << static_cast<int16_t>(rs) << std::endl;
		return rs;
	}
#endif

}


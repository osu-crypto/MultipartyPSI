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
#include "NTL/GF2EX.h"
#include "NTL/GF2XFactoring.h"
#include <NTL/GF2E.h>
#include "NTL/GF2EX.h"
#include <NTL/ZZ_pE.h>
#include <NTL/vec_ZZ_pE.h>
#include "NTL/GF2EX.h"
#include "NTL/ZZ_p.h"
#include "NTL/GF2EX.h" 
#include "NTL/GF2XFactoring.h"

#define TABLEb 0 
#define SepPOLYb 1 
#define OnePOLYb 2
#define BFb 3

namespace osuCrypto
{

	static int isSet(block & v, int n)
	{
		__m128i chkmask = _mm_slli_epi16(_mm_set1_epi16(1), n & 0xF);
		int     movemask = (1 << (n >> 3));
		int     isSet = (((_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_and_si128(chkmask, v), _mm_setzero_si128())) & movemask) ^ movemask));
		return isSet;
	}
	static 	void setBit(block & v, int pos)
	{
		__m128i shuf = _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
		shuf = _mm_add_epi8(shuf, _mm_set1_epi8(16 - (pos >> 3)));
		shuf = _mm_and_si128(shuf, _mm_set1_epi8(0x0F));
		__m128i setmask = _mm_shuffle_epi8(_mm_cvtsi32_si128(1 << (pos & 0x7)), shuf);
		v = _mm_or_si128(v, setmask);
	}
	static bool TestBitN(__m128i value, int N)
	{
		__m128i positioned = _mm_slli_epi64(value, 7 - (N & 7));
		return (_mm_movemask_epi8(positioned) & (1 << (N / 8))) != 0;
	}


	class TableBased
	{
	public:
		TableBased();
		~TableBased();


		u64 mRealBitSize, mMaxBitSize, mNumTrial;
		std::vector<u8> mPos; //key: bit location; value: index of 
		std::vector<u8> mMaps;

		void print() const;
		void init(/*u64 numRealCodeWord,*/ u64 numMaxBitSize);
		bool getMasks(std::vector<block>& codeword);
		void getMask(block& codeword, u8& mask);
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

	class BaseOPPRF
	{
	public:
		BaseOPPRF();
		~BaseOPPRF();



		NTL::GF2X mGf2x;
		u64 mNumBytes;

		void poly_init(u64 numBytes);

		//void findPos(std::vector<block>& codewords);
		//int isSet(block& codeword, int pos);
		//void setBit(block& codeword, int pos);
		//bool TestBitN(__m128i value, int N);

		//#################POLYNOMIAL
		void GF2EFromBlock(NTL::GF2E &element, block& blk, u64 size);
		void BlockFromGF2E(block& blk, NTL::GF2E & element, u64 size);
		//computes coefficients (in blocks) of f such that f(x[i]) = y[i]
		void getBlkCoefficients(NTL::vec_GF2E& setX, NTL::vec_GF2E& setY, std::vector<block>& coeffs);

		void getBlkCoefficients(u64 degree, std::vector<block>& setX, std::vector<block>& setY, std::vector<block>& coeffs);
		//compute y=f(x) giving coefficients (in block)
		void evalPolynomial(std::vector<block>& coeffs, block& x, block& y);
		NTL::GF2EX buildPolynomial(std::vector<block>& coeffs);
	};

}

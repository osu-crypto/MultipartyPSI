#pragma once
 
#include "Crypto/Commit.h"
#include "Crypto/PRNG.h"
#include "Common/Defines.h"
#include "Network/Channel.h"
using std::array;
#define SEED_SIZE_BYTES SEED_SIZE
namespace bOPRF {
	/*
	 * Generate a secure, random seed between 2 parties via commitment
	 */
	void random_seed_commit(u8* seed, Channel& channel, int len, const block& prngSeed);

	/*
	 * GF(2^128) multiplication using Intel instructions
	 * (should this go in gf2n class???)
	 */
	void gfmul128(__m128i a, __m128i b, __m128i *res);
	// Without reduction
	void mul128(__m128i a, __m128i b, __m128i *res1, __m128i *res2);
	void gfred128(__m128i a1, __m128i a2, __m128i *res);

	//#if defined(__SSE2__)
	/*
	 * Convert __m128i to string of type T
	 */
	template <typename T>
	std::string __m128i_toString(const __m128i var) {
		std::stringstream sstr;
		sstr << std::hex;
		const T* values = (const T*)&var;
		if (sizeof(T) == 1) {
			for (unsigned int i = 0; i < sizeof(__m128i); i++) {
				sstr << (int)values[i] << " ";
			}
		}
		else {
			for (unsigned int i = 0; i < sizeof(__m128i) / sizeof(T); i++) {
				sstr << values[i] << " ";
			}
		}
		return sstr.str();
	}
	//#endif
	void eklundh_transpose128(std::array<block, 128>& inOut);
	void sse_transpose128(std::array<block, 128>& inOut);
	std::string u64_to_bytes(const u64 w);

	//void shiftl128(u64 x1, u64 x2, u64& res1, u64& res2, size_t k);

}

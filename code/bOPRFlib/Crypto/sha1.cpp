/*
 * SHA1 routine optimized to do word accesses rather than byte accesses,
 * and to avoid unnecessary copies into the context array.
 *
 * This was initially based on the Mozilla SHA1 implementation, although
 * none of the original Mozilla code remains.
 */


#include "sha1.h"
#include "Common/Defines.h"
#include <string.h>

namespace bOPRF
{

	//void SHA1::Reset()
	//{
	//	mSha.Restart();
	//}

	////void blk_SHA1_Init(blk_SHA_CTX *ctx)
	////{
	////	size = 0;

	////	/* Initialize H with the magic constants (see FIPS180 for constants) */
	////	H[0] = 0x67452301;
	////	H[1] = 0xefcdab89;
	////	H[2] = 0x98badcfe;
	////	H[3] = 0x10325476;
	////	H[4] = 0xc3d2e1f0;
	////}

	//void SHA1::Update(const u8 *data, u64 len)
	//{
	//		
	//}

	//void SHA1::Final(u8* hashout)
	//{

	//	

	//	//static const unsigned char pad[64] = { 0x80 };
	//	//unsigned int padlen[2];
	//	//int i;

	//	///* Pad with a binary 1 (ie 0x80), then zeroes, then length */
	//	//padlen[0] = HTONL((uint32_t)(mSize >> 29));
	//	//padlen[1] = HTONL((uint32_t)(mSize << 3));

	//	//i = mSize & 63;
	//	//Update(pad, 1 + (63 & (55 - i)));
	//	//Update((u8*)padlen, 8);

	//	///* Output hash */
	//	//for (i = 0; i < 5; i++)
	//	//	put_be32(hashout + i * 4, mH[i]);
	//}
}

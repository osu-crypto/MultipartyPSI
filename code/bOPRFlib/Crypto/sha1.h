#pragma once

/*
 * SHA1 routine optimized to do u64 accesses rather than byte accesses,
 * and to avoid unnecessary copies into the context array.
 *
 * This was initially based on the Mozilla SHA1 implementation, although
 * none of the original Mozilla code remains.
 */
#include "Common/Defines.h"
#include "cryptopp/sha.h"

namespace bOPRF {
	class SHA1
	{
	public:
		static const u64 HashSize = 20;
		SHA1() { Reset(); }

		//u64 mSize;
		//u32 mH[5];
		//u32 mW[16];

		inline void Reset()
		{
			mSha.Restart();
		}
		inline void Update(const u8* dataIn, u64 length)
		{
			mSha.Update(dataIn, length);
		}
		inline void Update(const block& blk)
		{
			Update(ByteArray(blk), sizeof(block));
		}
		inline void Update(const blockBop& blk, u64 length)
		{
			Update(ByteArray(blk), length);
		}

		inline void Final(u8* DataOut)
		{
			mSha.Final(DataOut);
		}

		inline const SHA1& operator=(const SHA1& src)
		{
			mSha = src.mSha;
			return *this;
		}

	private:
		CryptoPP::SHA1 mSha;

	};
	class SHA2
	{
	public:
		static const u64 HashSize = 512;
		SHA2() { Reset(); }

		inline void Reset()
		{
			mSha.Restart();
		}
		inline void Update(const u8* dataIn, u64 length)
		{
			mSha.Update(dataIn, length);
		}
		inline void Update(const block& blk)
		{
			Update(ByteArray(blk), sizeof(block));
		}
		inline void Final(u8* DataOut)
		{
			mSha.Final(DataOut);
		}

	private:
		CryptoPP::SHA512 mSha;

	};

}

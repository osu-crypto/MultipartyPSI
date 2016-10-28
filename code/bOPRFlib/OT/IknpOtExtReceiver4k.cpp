#include "OT/Base/BaseOT.h"
#include "IknpOtExtReceiver4k.h"
#include "OT/Base/Tools.h"
#include "Common/Log.h"

using namespace std;

namespace bOPRF
{
//#define OTEXT_DEBUG
	void IknpOtExtReceiver4k::setBaseOts(ArrayView<std::array<block, 2>> baseOTs)
	{
		if (baseOTs.size() != BASE_OT_COUNT)
			throw std::runtime_error("rt error at " LOCATION);

		for (int i = 0; i < BASE_OT_COUNT; i++)
		{
			mGens[i][0].SetSeed(baseOTs[i][0]);
			mGens[i][1].SetSeed(baseOTs[i][1]);
		}


		mHasBase = true;
	}

	//length of PRC is around 4k, we extend baseOT to 4k 
	void IknpOtExtReceiver4k::Extend(
		const BitVector choices,
		std::array<std::array<block, BASE_OT_COUNT>, 4> &messages,
		PRNG& prng,
		Channel& chl)
	{
		if (choices.size() == 0) return;

		if (mHasBase == false)
			throw std::runtime_error("rt error at " LOCATION);

		auto numOTExt = 512;
		u64 numBlocks = 4; 
		u64 doneIdx = 0;

		std::array<std::array<block, BASE_OT_COUNT>, 4> t0;


		SHA1 sha;
		u8 hashBuff[SHA1::HashSize];

		// turn the choice vbitVector into an array of blocks. 
		BitVector choices2(numBlocks * 128);
		choices2 = choices;
		choices2.resize(numBlocks * 128);
		auto choiceBlocks = choices2.getArrayView<block>();

#ifdef OTEXT_DEBUG
		ByteStream debugBuff;
		chl.recv(debugBuff);
		block debugDelta; debugBuff.consume((u8*)&debugDelta, sizeof(block));

		Log::out << "delta" << Log::endl << debugDelta << Log::endl;
#endif 
		for (u64 blkIdx = 0; blkIdx < numBlocks; ++blkIdx)
		{			
			std::unique_ptr<ByteStream> uBuff(new ByteStream(BASE_OT_COUNT * sizeof(block)));
			uBuff->setp(BASE_OT_COUNT * sizeof(block));

			auto u = uBuff->getArrayView<block>();

			for (u64 colIdx = 0; colIdx < BASE_OT_COUNT; colIdx++)
			{
				// This is t0[colIdx]	
				t0[blkIdx][colIdx] = mGens[colIdx][0].get_block();
				
				// This is t1[colIdx]
				block t1i = mGens[colIdx][1].get_block();

				// compute the next column of u 
				u[colIdx] = t1i ^ (t0[blkIdx][colIdx] ^ choiceBlocks[blkIdx]);
			//	Log::out << "mGens[" << numBlocks << "," << colIdx << "]=" << t0[colIdx] << " --- " << t1i << Log::endl;
				//Log::out << "Receiver sent u[" << colIdx << "]=" << u[colIdx] <<" = " << t1i <<" + " << t0[colIdx] << " + " << choiceBlocks[blkIdx] << Log::endl;
			}

			// send over u buffer
			chl.asyncSend(std::move(uBuff));

			// transpose t0 in place
			//eklundh_transpose128(t0[blkIdx]);
			sse_transpose128(t0[blkIdx]);

#ifdef OTEXT_DEBUG 
			chl.recv(debugBuff); assert(debugBuff.size() == sizeof(t0));
			block* q = (block*)debugBuff.data();
#endif
			u32 blkRowIdx;
			for (blkRowIdx = 0; blkRowIdx < BASE_OT_COUNT; blkRowIdx++, doneIdx++)
			{
#ifdef OTEXT_DEBUG
				u8 choice = choices[doneIdx];
				block expected = choice ? (q[blkIdx][blkRowIdx] ^ debugDelta) : q[blkIdx][blkRowIdx];
				//Log::out << (int)choice << " " << expected << Log::endl;

				if (t0[blkIdx][blkRowIdx] != expected)
				{
					Log::out << "- " << t0[blkIdx][blkRowIdx] << Log::endl;
					throw std::runtime_error("rt error at " LOCATION);
				}
#endif

				// hash it
				sha.Reset();
				sha.Update((u8*)&t0[blkIdx][blkRowIdx], sizeof(block));
				sha.Final(hashBuff);
				messages[blkIdx][blkRowIdx] = *(block*)hashBuff;
			//	Log::out << (int)choices[doneIdx] << " " << messages[doneIdx] << Log::endl;
			}
		}		
	}
}

#include "IknpOtExtSender4k.h"

#include "OT/Base/BaseOT.h"
#include "OT/Base/Tools.h"
#include "Common/Log.h"

namespace bOPRF
{
//#define OTEXT_DEBUG

	using namespace std;

	void IknpOtExtSender4k::setBaseOts(ArrayView<block> baseRecvOts, const BitVector & choices)
	{
		if (baseRecvOts.size() != BASE_OT_COUNT || choices.size() != BASE_OT_COUNT)
			throw std::runtime_error("not supported/implemented");


		mBaseChoiceBits = choices;
		for (int i = 0; i < BASE_OT_COUNT; i++)
		{
			mGens[i].SetSeed(baseRecvOts[i]);
		}
	}

	//length of PRC is around 4k, we extend baseOT to 4k 
	void IknpOtExtSender4k::Extend(
		std::array<std::array<std::array<block,2>, BASE_OT_COUNT>, 4> &messages,
		PRNG& prng,
		Channel& chl)
	{
		if (messages.size() == 0) return;

		if (mBaseChoiceBits.size() != BASE_OT_COUNT)
			throw std::runtime_error("must set base first");

		SHA1 sha;
		u8 hashBuff[SHA1::HashSize];

		std::array<std::array<block, BASE_OT_COUNT>,4> q;
		block delta = *(block*)mBaseChoiceBits.data();
		ByteStream buff;
#ifdef OTEXT_DEBUG
		Log::out << "sender delta " << delta << Log::endl;
		buff.append(delta);
		chl.send(buff);
#endif

		u64 numBlocks = 4;
		for (u64 blkIdx = 0; blkIdx < numBlocks; ++blkIdx)
		{

			chl.recv(buff);
			assert(buff.size() == sizeof(block) * BASE_OT_COUNT);

			// u = t0 + t1 + x 
			auto u = buff.getArrayView<block>();

			for (int colIdx = 0; colIdx < BASE_OT_COUNT; colIdx++)
			{
				q[blkIdx][colIdx] = mGens[colIdx].get_block();
				//Log::out << "mGens[" << blkIdx << "," << colIdx << "]=" << mBaseChoiceBits[colIdx] << " --- " << q[colIdx] << Log::endl;
				if (mBaseChoiceBits[colIdx])
				{
					// now q[i] = t0[i] + Delta[i] * x
					q[blkIdx][colIdx] = q[blkIdx][colIdx] ^ u[colIdx];
				}
			}

			//eklundh_transpose128(q[blkIdx]);
			sse_transpose128(q[blkIdx]);

#ifdef OTEXT_DEBUG
			buff.setp(0);
			buff.append((u8*)&q, sizeof(q));
			chl.send(buff);
#endif
			//u32 stopIdx = std::min(u64(BASE_OT_COUNT), messages.size() - doneIdx);
			u32 RowIdx = 0;
			for (RowIdx < 0; RowIdx < BASE_OT_COUNT; RowIdx++)
			{
				auto& msg0 = q[blkIdx][RowIdx];
				auto msg1 = q[blkIdx][RowIdx] ^ delta;

				// hash message without delta
				sha.Reset();
				sha.Update((u8*)&msg0, sizeof(block));
				sha.Final(hashBuff);
				messages[blkIdx][RowIdx][0] = *(block*)hashBuff;

				// hash  message with delta
				sha.Reset();
				sha.Update((u8*)&msg1, sizeof(block));
				sha.Final(hashBuff);
				messages[blkIdx][RowIdx][1] = *(block*)hashBuff;

			}
		}
	}


}

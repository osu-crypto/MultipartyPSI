#include "OT/Base/BaseOT.h"
#include "SSOTReceiver.h"
#include "OT/Base/Tools.h"
#include "Common/Log.h"
#include  <mmintrin.h>

using namespace std;

namespace bOPRF
{
//#define OTEXT_DEBUG
//	#define	PRINT_OTEXT_DEBUG
	void SSOtExtReceiver::setBaseSSOts(std::array<std::array<std::array<block, 2>, BASE_OT_COUNT>, 4> baseSSOTs)
	{
		for (int blkIdx = 0; blkIdx < 4; blkIdx++) {
			if (baseSSOTs[blkIdx].size() != BASE_OT_COUNT)
				throw std::runtime_error("rt error at " LOCATION);

			for (int i = 0; i < BASE_OT_COUNT; i++)
			{
				mGens[blkIdx][i][0].SetSeed(baseSSOTs[blkIdx][i][0]);
				mGens[blkIdx][i][1].SetSeed(baseSSOTs[blkIdx][i][1]);
			}
		}
		mHasBase = true;
	}


	void SSOtExtReceiver::Extend(
		const u64 input_size,
		std::vector<std::array<blockBop, 2>>& sendMsg,
		Channel& chl)
	{	
		u64 doneIdx = 0;
		if (mHasBase == false)
			throw std::runtime_error("rt error at " LOCATION);
	
		auto numOTExt = ((input_size + 127) / 128) * 128;

		// we are going to process SSOTs in blocks of 128 messages.
		u64 numBlocks = numOTExt / BASE_OT_COUNT;

		// PRC length is around 4k
		std::array<std::array<block, BASE_OT_COUNT>,4> t0;
		std::array<std::array<block, BASE_OT_COUNT>, 4> t1;

	
		for (u64 blkIdx = 0; blkIdx < numBlocks; ++blkIdx)
		{
			for (u64 ssotBlkColIdx = 0; ssotBlkColIdx < 4; ssotBlkColIdx++)
			{

				for (u64 colIdx = 0; colIdx < BASE_OT_COUNT; colIdx++)
				{
					// use the base key from the base OTs to 
					// extend the i'th column of t0 and t1	
					t0[ssotBlkColIdx][colIdx] = mGens[ssotBlkColIdx][colIdx][0].get_block();
					t1[ssotBlkColIdx][colIdx] = mGens[ssotBlkColIdx][colIdx][1].get_block();
				}

				// transpose t0 in place
				sse_transpose128(t0[ssotBlkColIdx]);
				sse_transpose128(t1[ssotBlkColIdx]);
			}
			
			u32 stopIdx = std::min(u64(BASE_OT_COUNT), input_size - doneIdx);
			u32 blkRowIdx = 0;

			for (; blkRowIdx < stopIdx; ++blkRowIdx, ++doneIdx)
			{
				for (u64 ssotBlkColIdx = 0; ssotBlkColIdx < 4; ssotBlkColIdx++)
				{
					sendMsg[doneIdx][0].elem[ssotBlkColIdx] = t0[ssotBlkColIdx][blkRowIdx];
					sendMsg[doneIdx][1].elem[ssotBlkColIdx] = t1[ssotBlkColIdx][blkRowIdx];
					//Log::out << Log::lock;
					//Log::out << "s[" << doneIdx << "," << ssotBlkColIdx << ",0]--" << sendMsg[doneIdx][ssotBlkColIdx][0] << Log::endl;
					//Log::out << "s[" << doneIdx << "," << ssotBlkColIdx << ",1]--" << sendMsg[doneIdx][ssotBlkColIdx][1] << Log::endl;
					//Log::out << Log::unlock;
				}
				//sendMsg[doneIdx][0].elem3 =_mm_cvtsi128_si64(t0[3][blkRowIdx]);
				//sendMsg[doneIdx][1].elem3 = _mm_cvtsi128_si64(t1[3][blkRowIdx]);
			}
		}


#ifdef OTEXT_DEBUG
		ByteStream debugBuff;
		chl.recv(debugBuff);
		blockBop* debugDelta;
		debugBuff.consume((u8*)&debugDelta, codeWordSize);
		Log::out << Log::lock;  Log::out << "received debugDelta: " << debugDelta[0] << Log::endl; Log::out << Log::unlock;
		
#endif 
#ifdef OTEXT_DEBUG 
		debugBuff.setp(0);
		chl.recv(debugBuff);
		assert(debugBuff.size() == codeWordSize*sendMsg.size());
		auto q = debugBuff.getArrayView<blockBop>();
		Log::out << Log::lock;
		for (u64 rowIdx = 0; rowIdx < input_size; rowIdx++)
		{			
			blockBop tiui = sendMsg[rowIdx][0] ^ sendMsg[rowIdx][1];
			blockBop tiuis = tiui&debugDelta[0];
			blockBop c = sendMsg[rowIdx][0] ^ tiuis;


			if (neq(c, q[rowIdx]))
			{
				Log::out << "Incorrect extend columns of T, U, Q with a PRG" << Log::endl;
				Log::out << "I        have q_i " << c << Log::endl;
				Log::out << "but they have " << q[rowIdx] << Log::endl;

				throw std::runtime_error("Exit");;
			}
#ifdef PRINT_OTEXT_DEBUG 
			//Log::out << Log::lock;
			//Log::out << "C[" << rowIdx << "]=" << c << Log::endl;
			//Log::out << "q[" << rowIdx << "]=" << q[rowIdx] << Log::endl;
			Log::out << "ti[" << rowIdx << "]=" << sendMsg[rowIdx][0] << Log::endl;
			blockBop qtiuis = tiui^ q[rowIdx];
			Log::out << "qtiuis[" << rowIdx << "]=" << qtiuis << Log::endl;

		//	Log::out << Log::unlock;
#endif 
		}
		Log::out << Log::unlock;
#endif
	}
}

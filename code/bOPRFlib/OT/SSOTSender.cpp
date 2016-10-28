#include "SSOTSender.h"

#include "OT/Base/BaseOT.h"
#include "OT/Base/Tools.h"
#include "Common/Log.h"

namespace bOPRF
{
//#define OTEXT_DEBUG
//#define	PRINT_OTEXT_DEBUG
	using namespace std;

	void SSOtExtSender::setBaseSSOts(std::array<std::array<block, BASE_OT_COUNT>, 4> baseRecvOts, const BitVector & choices)
	{
		if (choices.size() != BASE_SSOT_COUNT)
			throw std::runtime_error("not supported/implemented");
		mBaseChoiceBits = choices;

		for (int blkIdx = 0; blkIdx < 4; blkIdx++) {
			if (baseRecvOts[blkIdx].size() != BASE_OT_COUNT )
				throw std::runtime_error("not supported/implemented");


			for (int i = 0; i < BASE_OT_COUNT; i++)
			{
				mGens[blkIdx][i].SetSeed(baseRecvOts[blkIdx][i]);
			}
		}
	}

	void SSOtExtSender::Extend(
		const u64 input_size,
		//std::vector<std::array<block, 4>> recvMsg,
		std::vector<blockBop>& recvMsg,
		Channel& chl)
	{
		// round up
		u64 numOTExt =  ((input_size + 127) / 128) * 128;

		// we are going to process SSOTs in blocks of 128 messages.
		u64 numBlocks = numOTExt / BASE_OT_COUNT;

		u64 doneIdx = 0;
		std::array<std::array<block, BASE_OT_COUNT>, 4> q;	

#ifdef OTEXT_DEBUG
		block* delta = (block*)mBaseChoiceBits.data();
		ByteStream buff;
		Log::out << "sender delta " << Log::endl;
		for (int i = 0; i<4; i++)
			Log::out << "delta[" << i << "]--" << delta[i] << Log::endl;

		buff.append((u8*)&delta, sizeof(q));
		chl.send(buff);
#endif

		for (u64 blkIdx =0; blkIdx < numBlocks; ++blkIdx)
		{
			for (u64 ssotBlkColIdx = 0; ssotBlkColIdx < 4; ssotBlkColIdx++)
			{

				for (int colIdx = 0; colIdx < BASE_OT_COUNT; colIdx++)
				{
					q[ssotBlkColIdx][colIdx] = mGens[ssotBlkColIdx][colIdx].get_block();

				}

				sse_transpose128(q[ssotBlkColIdx]);
			}		

			u32 stopIdx = std::min(u64(BASE_OT_COUNT), input_size - doneIdx);
			u32 blkRowIdx = 0;

			for (; blkRowIdx < stopIdx; ++blkRowIdx, ++doneIdx)
			{
				for (u64 ssotBlkColIdx = 0; ssotBlkColIdx < 4; ssotBlkColIdx++)
				{
					recvMsg[doneIdx].elem[ssotBlkColIdx] = q[ssotBlkColIdx][blkRowIdx];
				}
		}	
		}

#ifdef OTEXT_DEBUG
		buff.setp(0);
		buff.append((u8*)&recvMsg[0], sizeof(blockBop)*recvMsg.size());
		chl.send(buff);
		//block512* q1 = (blockBop*)buff.data();

#endif
#ifdef PRINT_OTEXT_DEBUG 
		Log::out << Log::lock;
		for (u64 rowIdx=0; rowIdx < input_size; rowIdx++)
		{			
			//Log::out << "r[" << rowIdx << "]--" << recvMsg[rowIdx] << Log::endl;	
			//Log::out << "q[" << rowIdx << "]--" << q1[rowIdx] << Log::endl;
			Log::out << "r[" << rowIdx << "]--" << recvMsg[rowIdx] << Log::endl;
		}
		Log::out << Log::unlock;
#endif
		
	
	}


}

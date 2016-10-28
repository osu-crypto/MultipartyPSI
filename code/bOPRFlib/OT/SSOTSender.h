#pragma once 
#include "OT/OTExtInterface.h"
#include "Common/BitVector.h"
#include "OT/Base/BaseOT.h"
#include "OT/BaseSSOT.h"

#include "Network/Channel.h"

#include <array>
#include <vector>
#ifdef GetMessage
#undef GetMessage
#endif
namespace bOPRF {

	class SSOtExtSender
	{
	public: 


		std::array<std::array<PRNG, BASE_OT_COUNT>,4> mGens;
		BitVector mBaseChoiceBits;

		bool hasBaseSSOts()// const override
		{
			return mBaseChoiceBits.size() > 0;
		}

		void setBaseSSOts(
			std::array<std::array<block, BASE_OT_COUNT>, 4> baseRecvOts,
			const BitVector& choices);// override;
		

		void Extend(
			const u64 input_size,
			std::vector<blockBop>& recvMsg,
			Channel& chl); //override;

	};
}


#pragma once
#include "OT/OTExtInterface.h"
#include "OT/Base/BaseOT.h"
#include "Network/Channel.h"
#include <vector>
#include "OT/BaseSSOT.h"

#ifdef GetMessage
#undef GetMessage
#endif

namespace bOPRF
{

	class SSOtExtReceiver// :
		//public OtExtReceiver
	{
	public:
		SSOtExtReceiver()
			:mHasBase(false)
		{}

		bool hasBaseSSOts() //const override
		{
			return mHasBase;
		}

		bool mHasBase;
		std::array<std::array<std::array<PRNG, 2>, BASE_OT_COUNT>,4> mGens;

		void setBaseSSOts(
			std::array<std::array<std::array<block, 2>, BASE_OT_COUNT>, 4> baseSendOts);// override;
		

		void Extend(
			const u64 input_size,
			//std::vector<std::array<std::array<block, 2>, 4>> sendMsg,
			std::vector<std::array<blockBop, 2>>& sendMsg,
			Channel& chl);// override;

	};

}

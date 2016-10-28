#pragma once
#include "OT/OTExtInterface.h"
#include "OT/Base/BaseOT.h"
#include "Network/Channel.h"
#include <vector>

#ifdef GetMessage
#undef GetMessage
#endif

namespace bOPRF
{

	class IknpOtExtReceiver4k// :
	{
	public:
		IknpOtExtReceiver4k()
			:mHasBase(false)
		{}

		bool hasBaseOts() //const override
		{
			return mHasBase;
		}

		bool mHasBase;
		std::array<std::array<PRNG, 2>, BASE_OT_COUNT> mGens;

		void setBaseOts(
			ArrayView<std::array<block, 2>> baseSendOts);// override;


		void Extend(
			const BitVector choices,
			std::array<std::array<block, BASE_OT_COUNT>, 4> &messages,
			PRNG& prng,
			Channel& chl);// override;

	};

}

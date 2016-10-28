#pragma once 
#include "OT/OTExtInterface.h"
#include "Common/BitVector.h"
#include "OT/Base/BaseOT.h"

#include "Network/Channel.h"

#include <array>
#include <vector>
#ifdef GetMessage
#undef GetMessage
#endif
namespace bOPRF {

	class IknpOtExtSender4k
	{
	public: 

		std::array<PRNG, BASE_OT_COUNT> mGens;
		BitVector mBaseChoiceBits;

		bool hasBaseOts() //const override
		{
			return mBaseChoiceBits.size() > 0;
		}

		void setBaseOts(
			ArrayView<block> baseRecvOts,
			const BitVector& choices);// override;


		void Extend(
			std::array<std::array<std::array<block, 2>, BASE_OT_COUNT>, 4>& messages,
			PRNG& prng,
			Channel& chl);// override;

	};
}


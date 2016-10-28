#pragma once
#include "Common/Defines.h"
#include "Common/BitVector.h"
#include "Crypto/PRNG.h"
#include <future>
//#include <vector>
#include "Common/ArrayView.h"

#ifdef GetMessage
#undef GetMessage
#endif
#define BASE_OT_COUNT 128

namespace bOPRF
{
	class OtExtReceiver
	{
	public:
		OtExtReceiver() {}

		virtual void setBaseOts(
			ArrayView<std::array<block,2>> baseSendOts) = 0;

		virtual bool hasBaseOts() const = 0;

		virtual void Extend(
			const BitVector choices,
			ArrayView<block> messages,
			PRNG& prng,
			Channel& chl)=0;

		void Extend(
			const BitVector choices,
			ArrayView<block> messages,
			PRNG& prng,
			Channel& chl)
		{
			Extend(choices, messages, prng, chl);
		}
	};

	class OtExtSender
	{
	public:
		OtExtSender() {}

		virtual bool hasBaseOts() const = 0;

		virtual void setBaseOts(
			ArrayView<block> baseRecvOts,
			const BitVector& choices)  = 0;


		virtual void Extend(
			ArrayView<std::array<block, 2>> messages,
			PRNG& prng,
			Channel& chl) = 0;


		virtual void Extend(
			ArrayView<std::array<block, 2>> messages,
			PRNG& prng,
			Channel& chl) 
		{
			Extend(messages, prng, chl);
		}
	};
}

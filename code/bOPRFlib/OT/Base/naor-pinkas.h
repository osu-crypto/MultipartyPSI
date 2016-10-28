#pragma once

#include "BaseOT.h"
#include "Common/ArrayView.h"
#include "Crypto/PRNG.h"

namespace bOPRF
{

	class NaorPinkas : public BaseOT
	{
	public:

		NaorPinkas();
		~NaorPinkas(); 
	void Receiver(ArrayView<block> messages, BitVector& choices, Channel& chl, PRNG& prng, u64 numThreads) override;
		void Sender(ArrayView<std::array<block, 2>> messages, Channel& sock, PRNG& prng, u64 numThreads) override;

	};

}

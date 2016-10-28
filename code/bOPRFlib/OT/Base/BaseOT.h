#pragma once

#include "Network/Channel.h"
#include "Common/BitVector.h"
#include "typedefs.h"
#include <ctime>
#include "miracl/include/big.h"

#include <iostream>
#include <cstring>
#include <fstream>
#include <time.h>
#include "Crypto/PRNG.h"
#include "Crypto/sha1.h"
#include "crypto/crypto.h"
namespace bOPRF
{


	Miracl* GetPrecision();
	Miracl* GetPrecision(int bits, int b);
	void deletePercision();

	class BaseOT
	{
	public:
		virtual void Receiver(ArrayView<block> messages, BitVector& choices, Channel& chl, PRNG& prng, u64 numThreads) = 0;
		virtual void Sender(ArrayView<std::array<block, 2>> messages, Channel& sock, PRNG& prng, u64 numThreads) = 0;

	};

}

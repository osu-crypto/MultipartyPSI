#pragma once

/* The OT thread uses the Miracl library, which is not thread safe.
* Thus all Miracl based code is contained in this one thread so as
* to avoid locking issues etc.
*
* Thus this thread serves all base OTs to all other threads
*/

#include "Crypto/PRNG.h"
#include "Common/BitVector.h"
#include "Network/Channel.h"
#include "Crypto/AES.h"
#include "OT/Base/BaseOT.h"
#include "OT/Base/PvwBaseOT.h"

#include <vector>

#define BASE_SSOT_COUNT 512

namespace bOPRF
{
class BaseSSOT
	{
	public:
		BitVector receiver_inputs;
		std::array<std::array<std::array<block, 2>, BASE_OT_COUNT>, 4> sender_inputs;
		std::array<std::array<block, BASE_OT_COUNT>, 4> receiver_outputs;
	Channel& mChannel;

		BaseSSOT(Channel& channel, OTRole role = Both)
			: mChannel(channel),
			nSSOT(BASE_SSOT_COUNT),
			ot_length(BASE_SSOT_COUNT),
			mOTRole(role)
		{
			receiver_inputs.reset(nSSOT);
		}

		int length() { return ot_length; }

		// do the OTs
		void exec_base(PRNG& prng);
		void check();
	private:
		int nSSOT, ot_length;
		OTRole mOTRole;

		bool is_sender() { return (bool)(mOTRole & Sender); }
		bool is_receiver() { return (bool)(mOTRole & Receiver); }
	};



}

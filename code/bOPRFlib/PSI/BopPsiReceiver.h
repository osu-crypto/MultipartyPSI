#pragma once

#include "Common/Defines.h"
#include "Network/Channel.h"
#include "OT/OTExtInterface.h"
#include "PSI/CuckooHasher.h"
#include "OT/SSOTReceiver.h"
#include "OT/SSOTSender.h"

namespace bOPRF
{

	class BopPsiReceiver
	{
	public:
		BopPsiReceiver();
		~BopPsiReceiver();

		u64 mN,mStatSecParam;
		std::vector<u64> mIntersection;
		CuckooHasher mBins;

		block mHashingSeed;
		
		std::vector<std::array<blockBop, 2>> mSSOtMessages; 

		u64 mNumStash;

		void init(u64 n, u64 statSecParam, Channel& chl0, SSOtExtReceiver& otRecv,  block seed);
		void init(u64 n, u64 statSecParam, const std::vector<Channel*>& chls, SSOtExtReceiver& otRecv,  block seed);
		void sendInput(std::vector<block>& inputs, Channel& chl);
		void sendInput(std::vector<block>& inputs, const std::vector<Channel*>& chls);

	};




}

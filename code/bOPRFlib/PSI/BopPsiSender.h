#pragma once
#include "Common/Defines.h"
#include "Network/Channel.h"
#include "OT/OTExtInterface.h"
#include "PSI/SimpleHasher.h"
#include "OT/SSOTReceiver.h"
#include "OT/SSOTSender.h"


namespace bOPRF
{


	class BopPsiSender
	{
	public:
		BopPsiSender();
		~BopPsiSender();

		u64 mN, mStatSecParam;
		
		//std::vector<SSOtPsiSender> mPsis;

		std::vector<blockBop> mPsiRecvSSOtMessages;



		SimpleHasher mBins;
		BitVector mSSOtChoice;

		block mHashingSeed;

		u64 mNumStash;

		void init(u64 n, u64 statSecParam, const std::vector<Channel*>& chls, SSOtExtSender& otSender, block seed);
		void init(u64 n, u64 statSecParam, Channel & chl0, SSOtExtSender& otSender, block seed);

		void sendInput(std::vector<block>& inputs, Channel& chl);
		void sendInput(std::vector<block>& inputs, const std::vector<Channel*>& chls);


	};

}
#include "OT/BaseSSOT.h"
#include "OT/Base/PvwBaseOT.h"
#include "Crypto/PRNG.h"
#include "Crypto/sha1.h"
#include "Network/Channel.h"
#include "cryptopp/osrng.h"
#include "Common/Log.h"
#include "OT/IknpOtExtReceiver4k.h"
#include "OT/IknpOtExtSender4k.h"

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include "OT/Base/naor-pinkas.h"

#include <boost/thread/tss.hpp>


using namespace std;
namespace bOPRF {
//#define PRINT_BASEOT_RESULT
	const char* role_to_str1(OTRole role)
	{
		if (role == Receiver)
			return "Receiver";
		if (role == Sender)
			return "Sender";
		return "Both";
	}





//	#define BASEOT_DEBUG

		// Run the PVW OTs
	void Exec_SSOT(
		std::array<std::array<std::array<block, 2>, BASE_OT_COUNT>, 4> &sendMsg,
		BitVector& SSOT_Base_Choice,
		std::array<std::array<block, BASE_OT_COUNT>, 4>& recvMsg,
		PRNG& G,
		Channel& channel,
		OTRole role)
	{
		u64 n = SSOT_Base_Choice.size();
	//	Log::out <<  "Starting base SSOTs as " << role_to_str1(role) << ", n = " << n << Log::endl;
		vector<ByteStream> strm(2);	
		IknpOtExtSender4k sender;
		IknpOtExtReceiver4k recv;

		//for BaseOT 128
		std::array<block, BASE_OT_COUNT> baseRecvMsg;
		BitVector baseChoices(BASE_OT_COUNT);
		baseChoices.randomize(G);
		std::array<std::array<block, 2>, BASE_OT_COUNT> baseSenderMsg;

		if (role & Receiver) 
		{					
			// Generate my receiver inputs 
			SSOT_Base_Choice.randomize(G);
			//PvwBaseOT baseOTs(channel, OTRole::Sender);
			//baseOTs.exec_base(G);
			//baseOTs.check();
			//	recv.setBaseOts(baseOTs.sender_inputs);
			

			NaorPinkas base;
			base.Sender(baseSenderMsg, channel, G, 2);		
			recv.setBaseOts(baseSenderMsg);
			recv.Extend(SSOT_Base_Choice, recvMsg, G, channel);
		}


		if (role & Sender)
		{
			// Generate my receiver inputs 
			/*PvwBaseOT baseOTs(channel, OTRole::Receiver);
			baseOTs.exec_base(G);
			baseOTs.check();*/
			NaorPinkas base;
			base.Receiver(baseRecvMsg, baseChoices, channel, G, 2);
			sender.setBaseOts(baseRecvMsg, baseChoices);
			sender.Extend(sendMsg, G, channel);
		}
	}


	void BaseSSOT::exec_base(PRNG& G)
	{
		Exec_SSOT(sender_inputs, receiver_inputs, receiver_outputs, G, mChannel, mOTRole);

#ifdef PRINT_BASEOT_RESULT
		for (u64 blkIdx = 0; blkIdx < 4; ++blkIdx)
		{
			for (int i = 0; i < BASE_OT_COUNT; i++)
			{
				if (mOTRole & Sender)
				{				
								Log::out << "[" << i << ",0]--" << sender_inputs[blkIdx][i][0] << Log::endl;
								Log::out << "[" << i << ",1]--" << sender_inputs[blkIdx][i][1] << Log::endl;
					
					}
					if (mOTRole & Receiver)						
							Log::out << (int)receiver_inputs[blkIdx*BASE_OT_COUNT+ i] << "-- " << receiver_outputs[blkIdx][i] << Log::endl;
					

				
			}
		}
#endif
	}


	void BaseSSOT::check()
	{
		ByteStream os;
		block tmp;
		for (u64 blkIdx = 0; blkIdx < 4; ++blkIdx)
		{
			for (int i = 0; i < BASE_OT_COUNT; i++)
			{
				if (mOTRole & Sender)
				{
					// send both inputs over
					os.append(sender_inputs[blkIdx][i][0]);
					os.append(sender_inputs[blkIdx][i][1]);

					if (eq(sender_inputs[blkIdx][i][0], sender_inputs[blkIdx][i][1]))
						throw std::runtime_error("rt error at " LOCATION);


					mChannel.asyncSendCopy(os);
				}

				if (mOTRole & Receiver)
				{
					mChannel.recv(os);

					os.consume((u8*)&tmp, sizeof(block));

					if (receiver_inputs[blkIdx*BASE_OT_COUNT+i] == 1)
					{
						os.consume((u8*)&tmp, sizeof(block));
					}

					if (neq(tmp, receiver_outputs[blkIdx][i]))
					{
						Log::out << "Base Incorrect OT" << Log::endl;
						Log::out << "I        have " << receiver_outputs[blkIdx][i] << Log::endl;
						Log::out << "but they have " << tmp << Log::endl;

						throw std::runtime_error("Exit");;
					}
				}
				os.setp(0);
			}
		}
	}
}



#include "EQ_Tests.h"

#include "Common.h"
#include "Network/BtEndpoint.h"
#include "Common/Defines.h"
#include "OPPRF/OPPRFReceiver.h"
#include "OPPRF/OPPRFSender.h"
#include "OPPRF/binSet.h"
#include "Common/Log.h"

#include "NChooseOne/KkrtNcoOtReceiver.h"
#include "NChooseOne/KkrtNcoOtSender.h"


#include "NChooseOne/Oos/OosNcoOtReceiver.h"
#include "NChooseOne/Oos/OosNcoOtSender.h"

#include "Hashing/CuckooHasher1.h"
#include "Hashing/BitPosition.h"
#include "Common/Log.h"
#include "Common/Log1.h"
#include <array>

#include "NTL/GF2EX.h"
#include "NTL/GF2XFactoring.h"
#include <NTL/GF2E.h>
#include "NTL/GF2EX.h"
#include <NTL/ZZ_pE.h>
#include <NTL/vec_ZZ_pE.h>
#include "NTL/GF2EX.h"
#include "NTL/ZZ_p.h"
#include "NTL/GF2EX.h" 
#include "NTL/GF2XFactoring.h"
#include "Common/Log.h"

using namespace osuCrypto;
#define PRINT
//#define BIN_PRINT

void EQ_EmptrySet_Test_Impl()
{
	Log::setThreadName("Sender");
	Log::setThreadName("CP_Test_Thread");
	std::string name("psi");
	u64 repeatCount = 1;
	u64 setPower = 8;
	u64 bitSize = 128;
	u64 setSize = pow(2, setPower);
	//u64 setSize = 10;
	u64 psiSecParam = 40, numThreads(1);

	BtIOService ios(0);
	BtEndpoint ep0(ios, "localhost", 1212, true, name);
	BtEndpoint ep1(ios, "localhost", 1212, false, name);


	std::vector<Channel*> recvChls(numThreads), sendChls(numThreads);
	for (u64 i = 0; i < numThreads; ++i)
		recvChls[i] = &ep1.addChannel(name + std::to_string(i), name + std::to_string(i));
	for (u64 i = 0; i < numThreads; ++i)
		sendChls[i] = &ep0.addChannel(name + std::to_string(i), name + std::to_string(i));

	PRNG prng(_mm_set_epi32(4253465, 34354565, 234435, 23987045));

	std::vector<u32> sendSet(setSize), recvSet(setSize);
	for (u64 i = 0; i < setSize; ++i)
	{
		sendSet[i] = prng.get<u32>();
		recvSet[i] = sendSet[i];
	}
	
	OPPRFSender send;
	OPPRFReceiver recv;
	binSet binSend;
	binSet binRecv;
		
	auto sendThrd = std::thread([&]() {
		binSend.init(0, 2, setSize, psiSecParam, 0);
	

	});

	auto recvThrd = std::thread([&]() {
		binRecv.init(1, 2, setSize, psiSecParam, 0);
	

	});

	sendThrd.join();
	recvThrd.join();

	for (u64 i = 0; i < numThreads; ++i)
	{
		sendChls[i]->close();
		recvChls[i]->close();
	}

	ep0.stop();
	ep1.stop();
	ios.stop();
}

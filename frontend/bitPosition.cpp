#include "bloomFilterMain.h"
#include "Network/BtEndpoint.h" 

#include "OPPRF/OPPRFReceiver.h"
#include "OPPRF/OPPRFSender.h"

#include <fstream>
using namespace osuCrypto;
#include "util.h"

#include "Common/Defines.h"
#include "NChooseOne/KkrtNcoOtReceiver.h"
#include "NChooseOne/KkrtNcoOtSender.h"

#include "NChooseOne/Oos/OosNcoOtReceiver.h"
#include "NChooseOne/Oos/OosNcoOtSender.h"
#include "Common/Log.h"
#include "Common/Log1.h"
#include "Common/Timer.h"
#include "Crypto/PRNG.h"
#include <numeric>
#include "bitPosition.h"
#include <set>

void Bit_Position_Test()
{
	u64 setSize = 1<<5;
	std::vector<block> testSet(setSize);
	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));

	for (u64 i = 0; i < setSize; ++i)
	{
		testSet[i] = prng.get<block>();
	}

	BitPosition b;
	b.init(setSize);
	std::cout << b.mSize << std::endl;
	std::set<u8> masks;
	Timer timer;
	auto start = timer.setTimePoint("start");
	b.findPos(testSet, masks);
	auto end = timer.setTimePoint("done");
	auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	//time = time / 100;
	std::cout << "time: " << time << " ms";
	std::cout << "\ntrial: " << b.mNumTrial;

	//auto start1 = timer.setTimePoint("start1");
	//b.findPos1(testSet, masks);
	//auto end1 = timer.setTimePoint("done");
	//auto time1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count();
	////time = time / 100;
	//std::cout << "\ntime: " << time1 << " ms";
	//std::cout << "\ntrial: " << b.mNumTrial;


	for (auto it = masks.begin(); it != masks.end(); ++it)
	{
		std::cout << *it << "  ";
	}

}


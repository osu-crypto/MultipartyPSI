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
	u64 setSize =32;
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


	/*for (auto it = masks.begin(); it != masks.end(); ++it)
	{
		std::cout << *it << "  ";
	}*/

}

void Bit_Position_Map_Test()
{
	u64 setSize = 1 << 7;
	std::vector<block> testSet(setSize);
	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));

	for (u64 i = 0; i < setSize; ++i)
	{
		testSet[i] = prng.get<block>();
	}

	BitPosition b;
	b.init(setSize);
	b.mSize = 7;
	std::set<u8>::iterator it;
	std::pair<std::set<u8>::iterator, bool> ret;

	for (u8 i = 0; i < b.mSize; i++)
	{
		u64 rand = std::rand() % 128; //choose randome bit location
		ret = b.mPos.insert(rand);
		if (ret.second == false)
			it = ret.first;
	}

	//for (u8 i = 0; i < setSize; i++)
	//{
	//	//u8 map1 = b.map(testSet[i]);
	//	//u8 map2 = b.map2(testSet[i]);
	//	//if (map1 != map2)
	//	{
	//		//std::cout << "map1!=map2" << std::endl;
	//		//std::cout << "map1: " << static_cast<int16_t>(map1) << std::endl;
	//		//std::cout << "map2: " << static_cast<int16_t>(map2) << std::endl;
	//	}
	//	
	//}
	Timer timer;
	auto start = timer.setTimePoint("start");
	for (u8 i = 0; i < setSize; i++)
	{
		u8 map2 = b.map2(testSet[i]);
	}
	auto end = timer.setTimePoint("done");
	auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	std::cout << "time1: " << time << " ms\n";

	start = timer.setTimePoint("start");
	for (u8 i = 0; i < setSize; i++)
	{
		u8 map1 = b.map(testSet[i]);
	}
	 end = timer.setTimePoint("done");
	time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	std::cout << "time2: " << time << " ms\n";

}

void Bit_Position_Recursive_Test()
{
	u64 setSize = 1 << 6;
	std::vector<block> testSet(setSize);
	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));

	for (u64 i = 0; i < setSize; ++i)
	{
		testSet[i] = prng.get<block>();
	}
	BitPosition b;
	b.init(setSize);
	std::set<int> rs;

	Timer timer;
	auto start = timer.setTimePoint("start");
	
	rs.insert(0);
	std::vector<block> testSet1;
	std::vector<block> testSet2;
	for (size_t i = 0; i < setSize; i++)
		if (b.TestBitN(testSet[i], 0))
			testSet1.push_back(testSet[i]);
		else
			testSet2.push_back(testSet[i]);
	
	b.getIdxs(testSet1, 128, rs, b.mSize);
	b.getIdxs(testSet2, 128, rs, b.mSize);


	auto end = timer.setTimePoint("done");
	auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	//time = time / 100;
	std::cout << "time: " << time << " ms\n";
	std::set<int>::iterator iter;
	for (iter = rs.begin(); iter != rs.end(); ++iter) {
		std::cout << (*iter) << " " << std::endl;
	}


}
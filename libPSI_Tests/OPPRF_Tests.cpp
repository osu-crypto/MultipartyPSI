#include "OPPRF_Tests.h"

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

u32 opt = 0;
void testPointer(std::vector<block>* test)
{
	//int length = test->size();
	//std::cout << length << std::endl;


	AES ncoInputHasher;

	ncoInputHasher.setKey(_mm_set1_epi64x(112434));
	ncoInputHasher.ecbEncBlocks((*test).data(), test->size() - 1, (*test).data());
	//Log::out << "mHashingSeed: " << mHashingSeed << Log::endl;




}
void testPointer2(std::vector<block>&  test)
{
	AES ncoInputHasher;

	ncoInputHasher.setKey(_mm_set1_epi64x(112434));
	ncoInputHasher.ecbEncBlocks(test.data(), test.size() - 1, test.data());



}

void Bit_Position_Test_Impl()
{
	std::cout << sizeof(u8) << std::endl;

	std::vector<int> myvector;

	// set some initial content:
	//for (int i = 1; i<10; i++) myvector.push_back(i);

	myvector.resize(0);

#if 0
	u64 setSize = 1 << 4;
	std::vector<block> testSet(setSize);
	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));

	for (u64 i = 0; i < setSize; ++i)
	{
		testSet[i] = prng.get<block>();
		std::cout << testSet[i] << std::endl;
	}
	testPointer2(testSet);
	for (u64 i = 0; i < setSize; ++i)
	{
		std::cout << testSet[i] << std::endl;
	}

	block test = ZeroBlock;
	test.m128i_i8[0] = 31;
	BaseOPPRF b;
	b.init(5);
	for (int i = 0; i < 3; ++i) b.mPos.insert(i);
	b.mPos.insert(6);
	b.mPos.insert(7);
	//std::cout << static_cast<int16_t>(b.map(test)) <<std::endl;
	//std::cout << static_cast<int16_t>(b.map2(test));


	BaseOPPRF b2;
	b2.init(setSize);
	//std::cout << "size: " << b2.mSize << std::endl;

	std::set<u8> masks;
	//	b2.findPos(testSet, masks);
		//std::cout << "\nmNumTrial: " << b2.mNumTrial << std::endl;


	for (u8 i = 0; i < masks.size(); i++)
	{
		//std::cout << static_cast<int16_t>(masks[i]) << std::endl;
	}
#endif
}

double maxprob1(u64 balls, u64 bins, u64 k)
{
	return std::log(bins * std::pow(balls * exp(1) / (bins * k), k)) / std::log(2);
}
u64 findMaxBinSize(u64 n, u64 numBins, u64 numHash = 2)
{
	u64 balls = numHash*n;
	u64 maxBin;
	for (maxBin = 15; maxBin < 64; maxBin++)
	{
		// finds the min number of bins needed to get max occ. to be maxBin
		if (-maxprob1(balls, numBins, maxBin) < 40)
		{
			// maxBins is too small, skip it.
			continue;
		}
		else
			return maxBin;
	}
}
double findScaleNumBins(u64 n, u64 maxBin, u64 numHash = 2)
{
	u64 balls = numHash*n;
	double scale;
	for (scale = 0.01; scale < 1; scale += 0.01)
	{
		// finds the min number of bins needed to get max occ. to be maxBin
		if (-maxprob1(balls, scale*n, maxBin) < 40)
		{
			// maxBins is too small, skip it.
			continue;
		}
		else
			return scale;
	}
}
void findScaleNumBins_Test_Impl()
{
	u64 n = 1 << 12;
	u64 maxBin = 63;

	double scaleNumBins = findScaleNumBins(n, maxBin);
	std::cout << scaleNumBins << std::endl;
}
void findMaxBinSize_Test_Impl()
{
	//u64 n = 1 << 12;

	std::vector<u64> n = { 12,14,16,20,24 };
	std::vector<double> scale30 = { 1.14 ,1.12,1.12,1.12,1.11 };
	std::vector<double> scale40 = {1.17 ,1.14,1.13,1.12,1.11 };

	

	std::cout << "2^-30 | \n";
	for (u64 i = 0; i < n.size(); i++)
	{
		u64 p = 1 << n[i];
		u64 numBins = scale30[i]*p;
		u64 maxBin = findMaxBinSize(p, numBins,3);
		std::cout << n[i] << " | "<< maxBin << std::endl;
	}

	std::cout << "2^-40 | \n";
	for (u64 i = 0; i < n.size(); i++)
	{
		u64 p = 1 << n[i];
		u64 numBins = scale40[i] * p;
		u64 maxBin = findMaxBinSize(p, numBins,3);
		std::cout << n[i] << " | " << maxBin << std::endl;
	}
	
}

void hashing2Bins_Test_Impl()
{
	u64 setSize = 1 << 5, psiSecParam = 40, bitSize = 128, numParties = 2;
	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));

	std::vector<block> set(setSize);

	for (u64 i = 0; i < setSize; ++i)
	{
		set[i] = prng.get<block>();
	}
	std::vector<std::thread>  pThrds(2);

	std::vector<binSet> bins(2);
	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
	{
		pThrds[pIdx] = std::thread([&, pIdx]() {


			bins[pIdx].init(pIdx, 2, setSize, psiSecParam,opt);
			bins[pIdx].hashing2Bins(set, 2); });
	}



	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
		pThrds[pIdx].join();

	bins[0].mSimpleBins.print(0, true, false, false, false);
	bins[1].mCuckooBins.print(1, true, false, false);

}

void Bit_Position_Recursive_Test_Impl()
{
	u64 setSize = 15;
	std::vector<block> testSet(setSize);
	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));

	for (u64 i = 0; i < setSize; ++i)
	{
		testSet[i] = prng.get<block>();
	}
	//for (u64 i = 0; i < 8; ++i)
	//{
	//	testSet[i].m128i_u8[0] = 1 << i;
	//}
	//for (u64 i = 8; i < setSize; ++i)
	//{
	//	testSet[i].m128i_u16[2] = 1 << i;
	//}

	BitPosition b;

#if 0
	block test = ZeroBlock;
	//test.m128i_i8[0] = 126;

	BaseOPPRF b;
	b.init(5, 5);
	for (size_t i = 0; i < 8; i++)
	{
		b.setBit(test, i);
		std::cout << b.isSet(testSet[10], i) << " ";
	}
	/*std::vector<int> cnt(128);
	int idx = 0;
	int mid = setSize / 2;
	for (size_t j = 0; j < 128; j++)
	{
	cnt[j] = 0;
	for (size_t i = 0; i < setSize; i++)
	{
	if (b.TestBitN(testSet[i], j))
	cnt[j]++;
	}
	if (cnt[j] < setSize / 2 && mid < cnt[j])
	{
	mid = cnt[j];
	idx = j;
	}
	std::cout << j << ": " << cnt[j] << std::endl;
	}*/
	//auto const it = std::lower_bound(cnt.begin(), cnt.end(), mid);
	//if (it == cnt.end())
	//{
	//	idx = 1;
	//}
	//else
	//	idx = *(it);
#endif
	std::set<int> rs;
	b.init(4);
	b.getPos1(testSet, 128);
	//b.getMasks(testSet);

	Log::out << "    getPos=";
	for (u64 j = 0; j < b.mPos.size(); ++j)
	{
		Log::out << static_cast<int16_t>(b.mPos[j]) << " ";
	}
	Log::out << Log::endl;

	for (u64 j = 0; j < b.mMaps.size(); ++j)
	{
		Log::out << testSet[j] << " " << static_cast<int16_t>(b.mMaps[j]) << " " << Log::endl;
	}
	Log::out << Log::endl;

}

void Bit_Position_Random_Test_Impl()
{
	u64 power = 5;
	u64 setSize = 1 << power;

	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));


	SimpleHasher1 mSimpleBins;
	mSimpleBins.init(setSize,0);
	std::vector<u64> tempIdxBuff(setSize);
	MatrixView<u64> hashes(setSize, mSimpleBins.mNumHashes[0]);

	for (u64 j = 0; j < setSize; ++j)
	{
		tempIdxBuff[j] = j;
		for (u64 k = 0; k < mSimpleBins.mNumHashes[0]; ++k)
		{
			block a = prng.get<block>();
			hashes[j][k] = *(u64*)&a;
		}
	}

	mSimpleBins.insertBatch(tempIdxBuff, hashes);

	for (u64 bIdx = 0; bIdx < mSimpleBins.mBins.size(); ++bIdx)
	{
		auto& bin = mSimpleBins.mBins[bIdx];
		if (bin.mIdx.size() > 0)
		{
			bin.mValOPRF.resize(1);
			bin.mBits.resize(1);
			bin.mValOPRF[0].resize(bin.mIdx.size());

			for (u64 i = 0; i < bin.mIdx.size(); ++i)
			{
				bin.mValOPRF[0][i] = prng.get<block>();
			}
		}
	}

	Timer mTimer;
	double mTime = 0;

	auto start = mTimer.setTimePoint("getPos1.start");

	for (u64 bIdx = 0; bIdx < mSimpleBins.mBinCount[0]; ++bIdx)
	{
		auto& bin = mSimpleBins.mBins[bIdx];
		if (bin.mIdx.size() > 0)
		{
			bin.mBits[0].init(mSimpleBins.mNumBits[0]);
			bin.mBits[0].getPos1(bin.mValOPRF[0], 128);
		}

	}
	auto mid = mTimer.setTimePoint("getPos1.mid");

	for (u64 bIdx = 0; bIdx < mSimpleBins.mBinCount[1]; ++bIdx)
	{
		auto& bin = mSimpleBins.mBins[mSimpleBins.mBinCount[0] + bIdx];
		if (bin.mIdx.size() > 0)
		{
			bin.mBits[0].init(mSimpleBins.mNumBits[1]);
			bin.mBits[0].getPos1(bin.mValOPRF[0], 128);
		}
	}

	auto end = mTimer.setTimePoint("getPos1.done");
	double time1 = std::chrono::duration_cast<std::chrono::milliseconds>(mid - start).count();
	double time2 = std::chrono::duration_cast<std::chrono::milliseconds>(end - mid).count();
	double time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	Log::out << "time1= " << time1 << "\n";
	Log::out << "time2= " << time2 << "\n";
	Log::out << "total= " << time << "\n";

	mSimpleBins.print(0, true, true, true, true);
	/*BaseOPPRF b;

	std::set<int> rs;
	b.init(4);
	Timer mTimer;
	auto start = mTimer.setTimePoint("getPos1.start");
	b.getPos1(testSet, 128);
	auto end = mTimer.setTimePoint("getPos1.done");
	double time= std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	Log::out << "time= " << time << "\n";

	Log::out << "    getPos= ";
	for (u64 j = 0; j < b.mPos.size(); ++j)
	{
		Log::out << static_cast<int16_t>(b.mPos[j]) << " ";
	}
	Log::out << Log::endl;

	for (u64 j = 0; j < b.mMaps.size(); ++j)
	{
		Log::out << testSet[j] << " " << static_cast<int16_t>(b.mMaps[j]) << " " << Log::endl;
	}*/
	Log::out << Log::endl;

}

void OPPRF_CuckooHasher_Test_Impl()
{
#if 0
	u64 setSize = 10000;

	u64 h = 2;
	std::vector<u64> _hashes(setSize * h + 1);
	MatrixView<u64> hashes(_hashes.begin(), _hashes.end(), h);
	PRNG prng(ZeroBlock);

	for (u64 i = 0; i < hashes.size()[0]; ++i)
	{
		for (u64 j = 0; j < h; ++j)
		{
			hashes[i][j] = prng.get<u64>();
		}
	}

	CuckooHasher hashMap0;
	CuckooHasher hashMap1;
	CuckooHasher::Workspace w(1);

	hashMap0.init(setSize, 40, 1, true);
	hashMap1.init(setSize, 40, 1, true);


	for (u64 i = 0; i < setSize; ++i)
	{
		//if (i == 6) hashMap0.print();

		hashMap0.insert(i, hashes[i]);

		std::vector<u64> tt{ i };
		MatrixView<u64> mm(hashes[i].data(), 1, 2, false);
		hashMap1.insertBatch(tt, mm, w);


		//if (i == 6) hashMap0.print();
		//if (i == 6) hashMap1.print();

		//if (hashMap0 != hashMap1)
		//{
		//    std::cout << i << std::endl;

		//    throw UnitTestFail();
		//}

	}

	if (hashMap0 != hashMap1)
	{
		throw UnitTestFail();
	}
#endif
}

void Channel_Test_Impl() {
	std::string name("psi");
	u64 numParties = 2;
	BtIOService ios(0);


	int btCount = numParties*(numParties);
	std::vector<BtEndpoint> ep(btCount);
	std::vector<std::vector<std::vector<Channel*>>> chls(numParties);

	//for (u64 i = 0; i < numParties; ++i)
	//{
	//	//ep[i].resize(numParties);
	//	chls[i].resize(numParties);

	//	for (u64 j = 0; j < numParties; ++j)
	//	{
	//		if (i < j) {
	//			auto port = i*10+j; //make same port: i=1, j=2 => port 12
	//			ep[i*numParties+j].start(ios, "localhost", port, true, name);
	//			//chls[i][j] = { &ep[i*numParties + j].addChannel(name, name) };

	//		}
	//		else if (i > j)
	//		{
	//			auto port = j*10+i; //i=2, j=1 => port 12
	//			ep[i*numParties + j].start(ios, "localhost", port, false, name);
	//			//chls[i][j] = { &ep[i*numParties + j].addChannel(name, name) };

	//		}

	//	}
	//}

	//for (u64 i = 0; i < numParties; ++i)
	//{
	//	chls[i].resize(numParties);
	//	for (u64 j = 0; j < numParties; ++j)
	//	{
	//		if (i != j) {
	//			chls[i][j].push_back(&ep[i*numParties + j].addChannel(name, name));

	//		}
	//	}
	//}
	BtEndpoint ep0;
	BtEndpoint ep1;
	ep[1].start(ios, "localhost", 1, true, name);
	ep[2].start(ios, "localhost", 1, false, name);
	std::vector<Channel*> recvChl{ &ep[2].addChannel(name, name) };
	std::vector<Channel*> sendChl{ &ep[1].addChannel(name, name) };


	std::vector<std::thread>  thrds(2);
	u8 dummy[1];
	u8 dummy1[1];
	for (u64 tIdx = 0; tIdx < thrds.size(); ++tIdx)
	{
		thrds[tIdx] = std::thread([&, tIdx]()
		{
			dummy[0] = tIdx + 5;
			if (tIdx % 2 == 0)
				//chls[0][1][0]->asyncSend(dummy, 1);
				sendChl[0]->asyncSend(dummy, 1);
			else
			{
				//chls[1][0][0]->recv(dummy1, 1);
				recvChl[0]->recv(dummy, 1);
				std::cout << static_cast<int16_t>(dummy[0]) << " ";

			}
		});
	}

	for (auto& thrd : thrds)
		thrd.join();

	chls[0][1][0]->close();
	chls[1][0][0]->close();

	for (u64 i = 0; i < numParties; ++i)
	{
		for (u64 j = 0; j < numParties; ++j)
		{
			if (i != j)
				ep[i*numParties + j].stop();
		}
	}

	ios.stop();
}
void OPPRF2_EmptrySet_Test_Impl_draft()
{
	u64 setSize = 1 << 5, psiSecParam = 40, bitSize = 128, numParties = 2;
	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));

	std::vector<block> sendSet(setSize), recvSet(setSize);
	std::vector<block> sendPayLoads(setSize), recvPayLoads(setSize);

	for (u64 i = 0; i < setSize; ++i)
	{
		sendSet[i] = prng.get<block>();
		sendPayLoads[i] = prng.get<block>();
		recvSet[i] = prng.get<block>();
		recvSet[i] = sendSet[i];
	}
	for (u64 i = 1; i < 3; ++i)
	{
		recvSet[i] = sendSet[i];
	}

	std::string name("psi");

	BtIOService ios(0);
	BtEndpoint ep0(ios, "localhost", 1212, true, name);
	BtEndpoint ep1(ios, "localhost", 1212, false, name);


	std::vector<Channel*> recvChl{ &ep1.addChannel(name, name) };
	std::vector<Channel*> sendChl{ &ep0.addChannel(name, name) };

	KkrtNcoOtReceiver otRecv0, otRecv1;
	KkrtNcoOtSender otSend0, otSend1;



	OPPRFSender send;
	OPPRFReceiver recv;
	/*   std::thread thrd([&]() {
	  
	   });*/
  



	sendChl[0]->close();
	recvChl[0]->close();

	ep0.stop();
	ep1.stop();
	ios.stop();
}
void OPPRF_EmptrySet_Test_Impl()
{
	u64 setSize = 1 << 5, psiSecParam = 40, bitSize = 128, numParties = 3;
	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));

	std::vector<block> sendSet(setSize), recvSet(setSize);
	std::vector<block> sendPayLoads(setSize), recvPayLoads(setSize);

	for (u64 i = 0; i < setSize; ++i)
	{
		sendSet[i] = prng.get<block>();
		sendPayLoads[i] = prng.get<block>();
		recvSet[i] = prng.get<block>();
		recvSet[i] = sendSet[i];
	}
	for (u64 i = 1; i < 3; ++i)
	{
		recvSet[i] = sendSet[i];
	}

	std::string name("psi");

	BtIOService ios(0);
	BtEndpoint ep01(ios, "localhost", 1212, true, name);
	BtEndpoint ep10(ios, "localhost", 1212, false, name);

	BtEndpoint ep02(ios, "localhost", 1213, true, name);
	BtEndpoint ep20(ios, "localhost", 1213, false, name);



	std::vector<Channel*> recvChl{ &ep10.addChannel(name, name) };
	std::vector<Channel*> sendChl{ &ep01.addChannel(name, name) };

	std::vector<Channel*> recvChl2{ &ep20.addChannel(name, name) };
	std::vector<Channel*> sendChl2{ &ep02.addChannel(name, name) };


	KkrtNcoOtReceiver otRecv0, otRecv1;
	KkrtNcoOtSender otSend0, otSend1;
	OPPRFSender send;
	OPPRFReceiver recv;

	KkrtNcoOtReceiver otRecv02, otRecv12;
	KkrtNcoOtSender otSend02, otSend12;
	OPPRFSender send2;
	OPPRFReceiver recv2;

	Log::out << "sendPayLoads[i]" << Log::endl;
	std::vector<std::thread>  pThrds(3);

	/*for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
	{
		pThrds[pIdx] = std::thread([&, pIdx]() {
			if (pIdx == 0)
			{
				send.init(opt, numParties, setSize, psiSecParam, bitSize, sendChl, otSend0, otRecv1, prng.get<block>());
				send2.init(opt, numParties, setSize, psiSecParam, bitSize, sendChl2, otSend02, otRecv12, prng.get<block>());

			}
			else if (pIdx == 1) {
				recv.init(opt, numParties, setSize, psiSecParam, bitSize, recvChl, otRecv0, otSend1, ZeroBlock);
			}
			else if (pIdx == 2) {
				recv2.init(opt, numParties, setSize, psiSecParam, bitSize, recvChl2, otRecv02, otSend12, ZeroBlock);
			}
		});
	}

	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
		pThrds[pIdx].join();*/


		/*std::thread thrd([&]() {
			send.init(opt, numParties, setSize, psiSecParam, bitSize, sendChl, otSend0, otRecv1, prng.get<block>());
		});
		recv.init(opt, numParties, setSize, psiSecParam, bitSize, recvChl, otRecv0, otSend1, ZeroBlock);

			thrd.join();*/

#ifdef PRINT
			//std::cout << IoStream::lock;
			//for (u64 i = 1; i < recvPayLoads.size(); ++i)
			//{
			//	Log::out << recvPayLoads[i] << Log::endl;
			//	Log::out << sendPayLoads[i] << Log::endl;
			//	if (memcmp((u8*)&recvPayLoads[i], &sendPayLoads[i], sizeof(block)))
			//	{
			//		Log::out << "recvPayLoads[i] != sendPayLoads[i]" << Log::endl;
			//		Log::out << recvSet[i] << Log::endl;
			//		Log::out << sendSet[i] << Log::endl;
			//		Log::out << i << Log::endl;
			//	}

			//}

			//std::cout << IoStream::unlock;

	std::cout << IoStream::lock;
	Log::out << otSend0.mT.size()[1] << Log::endl;
	Log::out << otSend1.mT.size()[1] << Log::endl;
	Log::out << otSend0.mGens[0].get<block>() << Log::endl;
	Log::out << otRecv0.mGens[0][0].get<block>() << Log::endl;
	Log::out << otRecv0.mGens[0][1].get<block>() << Log::endl;
	Log::out << "------------" << Log::endl;
	Log::out << otSend1.mGens[0].get<block>() << Log::endl;
	Log::out << otRecv1.mGens[0][0].get<block>() << Log::endl;
	Log::out << otRecv1.mGens[0][1].get<block>() << Log::endl;
	Log::out << "------------" << Log::endl;
	Log::out << otSend02.mGens[0].get<block>() << Log::endl;
	Log::out << otRecv02.mGens[0][0].get<block>() << Log::endl;
	Log::out << otRecv02.mGens[0][1].get<block>() << Log::endl;
	Log::out << "------------" << Log::endl;
	Log::out << otSend12.mGens[0].get<block>() << Log::endl;
	Log::out << otRecv12.mGens[0][0].get<block>() << Log::endl;
	Log::out << otRecv12.mGens[0][1].get<block>() << Log::endl;

	std::cout << IoStream::unlock;

#endif

	//	thrd.join();



	sendChl[0]->close();
	recvChl[0]->close();
	sendChl2[0]->close();
	recvChl2[0]->close();

	ep01.stop();
	ep10.stop();
	ep02.stop();
	ep20.stop();
	ios.stop();
}

void OPPRF3_EmptrySet_Test_Impl_draft()
{
	u64 setSize = 1 << 5, psiSecParam = 40, bitSize = 128, numParties = 3;
	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));

	std::vector<std::vector<block>> set(numParties);
	std::vector<std::vector<block>> sendPayLoads(numParties), recvPayLoads(numParties);

	for (u64 i = 0; i < setSize; ++i)
	{
		set[0][i] = prng.get<block>();
		set[1][i] = set[0][i];
		set[2][i] = set[0][i];
	}
	for (u64 idxP = 0; idxP < numParties; ++idxP)
	{
		sendPayLoads[idxP].resize(setSize);
		recvPayLoads[idxP].resize(setSize);
		for (u64 i = 0; i < setSize; ++i)
			sendPayLoads[idxP][i] = prng.get<block>();
	}

	std::string name("psi");

	BtIOService ios(0);
	BtEndpoint ep01(ios, "localhost", 1212, true, name);
	BtEndpoint ep10(ios, "localhost", 1212, false, name);
	std::vector<Channel*> recvChl10{ &ep10.addChannel(name, name) };
	std::vector<Channel*> sendChl01{ &ep01.addChannel(name, name) };

	BtEndpoint ep02(ios, "localhost", 1213, true, name);
	BtEndpoint ep20(ios, "localhost", 1213, false, name);
	std::vector<Channel*> recvChl20{ &ep20.addChannel(name, name) };
	std::vector<Channel*> sendChl02{ &ep02.addChannel(name, name) };


	std::vector<KkrtNcoOtReceiver> otRecv0(numParties);
	std::vector<KkrtNcoOtSender> otSend0(numParties);

	std::vector<KkrtNcoOtReceiver> otRecv1(numParties);
	std::vector<KkrtNcoOtSender> otSend1(numParties);

	std::vector<KkrtNcoOtReceiver> otRecv2(numParties);
	std::vector<KkrtNcoOtSender> otSend2(numParties);

	OPPRFSender send01, send02;

	OPPRFReceiver recv10, recv20;

	std::vector<OPPRFSender> send(2);
	std::vector<OPPRFReceiver> recv(2);

	std::vector<binSet> bins(3);

	std::vector<std::thread>  pThrds(numParties);

	//for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
	//{
	//	pThrds[pIdx] = std::thread([&, pIdx]() {
	//		if(pIdx==0)
	//		{
	//			send01.init(opt, numParties, setSize, psiSecParam, bitSize, sendChl01, otSend0[1], otRecv0[1], prng.get<block>());
	//		//	send02.init(opt, numParties, setSize, psiSecParam, bitSize, sendChl02, otSend0[2], otRecv0[2], prng.get<block>());

	//		}
	//		else if (pIdx == 1) {
	//			recv10.init(opt, numParties, setSize, psiSecParam, bitSize, recvChl10, otRecv1[0], otSend1[0], ZeroBlock);

	//		}
	//		else
	//		{ 
	//		//	recv20.init(opt, numParties, setSize, psiSecParam, bitSize, recvChl20, otRecv2[0], otSend2[0], ZeroBlock);

	//		}
	//	});
	//}
	//
	//for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
	//	pThrds[pIdx].join();


	/*Log::out << "recv.mCuckooBins.print(true, false, false);" << Log::endl;
	bins[1].mCuckooBins.print(0, true, true, true);*/

	//Log::out << "recv.mSimpleBins.print(true, false, false,false);" << Log::endl;
	//recv.mSimpleBins.print(0,true, true, true, true);

	/*recv.sendInput(recvSet, recvChl);
	recv.mBins.print();
	*/

#ifdef PRINT
	/*std::cout << IoStream::lock;
	for (u64 i = 1; i < recvPayLoads.size(); ++i)
	{
		Log::out << recvPayLoads[i] << Log::endl;
		Log::out << sendPayLoads[i] << Log::endl;
		if (memcmp((u8*)&recvPayLoads[i], &sendPayLoads[i], sizeof(block)))
		{
			Log::out << "recvPayLoads[i] != sendPayLoads[i]" << Log::endl;
			Log::out << recvSet[i] << Log::endl;
			Log::out << sendSet[i] << Log::endl;
			Log::out << i << Log::endl;
		}

	}

	std::cout << IoStream::unlock;

	std::cout << IoStream::lock;
	Log::out << otSend[0].mT.size()[1] << Log::endl;
	Log::out << otSend[1].mT.size()[1] << Log::endl;
	Log::out << otSend[0].mGens[0].get<block>() << Log::endl;
	Log::out << otRecv[0].mGens[0][0].get<block>() << Log::endl;
	Log::out << otRecv[0].mGens[0][1].get<block>() << Log::endl;
	Log::out << "------------" << Log::endl;
	Log::out << otSend[1].mGens[0].get<block>() << Log::endl;
	Log::out << otRecv[1].mGens[0][0].get<block>() << Log::endl;
	Log::out << otRecv[1].mGens[0][1].get<block>() << Log::endl;
	std::cout << IoStream::unlock;*/

#endif

	//	thrd.join();






	sendChl01[0]->close();
	sendChl02[0]->close();
	recvChl10[0]->close();
	recvChl20[0]->close();

	ep10.stop();  ep01.stop();  ep20.stop(); ep02.stop();
	ios.stop();
}

void OPPRF_EmptrySet_hashing_Test_Impl()
{
//	u64 setSize = 1 << 5, psiSecParam = 40, bitSize = 128, numParties = 2;
//	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
//
//	std::vector<block> sendSet(setSize), recvSet(setSize);
//	std::vector<block> sendPayLoads(setSize), recvPayLoads(setSize);
//
//	for (u64 i = 0; i < setSize; ++i)
//	{
//		sendSet[i] = prng.get<block>();
//		sendPayLoads[i] = prng.get<block>();
//		recvSet[i] = prng.get<block>();
//		recvSet[i] = sendSet[i];
//	}
//	for (u64 i = 1; i < 3; ++i)
//	{
//		recvSet[i] = sendSet[i];
//	}
//
//	std::string name("psi");
//
//	BtIOService ios(0);
//	BtEndpoint ep0(ios, "localhost", 1212, true, name);
//	BtEndpoint ep1(ios, "localhost", 1212, false, name);
//
//
//	std::vector<Channel*> recvChl{ &ep1.addChannel(name, name) };
//	std::vector<Channel*> sendChl{ &ep0.addChannel(name, name) };
//
//	std::vector<KkrtNcoOtReceiver> otRecv(2);
//	std::vector<KkrtNcoOtSender> otSend(2);
//
//	std::vector<OPPRFSender> send(1);
//	std::vector<OPPRFReceiver> recv(1);
//
//	std::vector<binSet> bins(2);
//
//
//
//	std::thread thrd([&]() {
//		bins[0].init(0, numParties, setSize, psiSecParam,opt);
//		u64 otCountSend = bins[0].mSimpleBins.mBins.size();
//		send[0].init(opt, numParties, setSize, psiSecParam, bitSize, sendChl, otCountSend, otSend[0], otRecv[0], prng.get<block>());
//
//
//		bins[0].hashing2Bins(sendSet, 2);
//		//send.hash2Bins(sendSet, sendChl);
//		send[0].getOPRFKeys(1, bins[0], sendChl, true);
//		send[0].sendSS(1, bins[0], sendPayLoads, sendChl);
//		//send.recvSS(1, recvPayLoads, sendChl);
//		//Log::out << "send.mSimpleBins.print(true, false, false,false);" << Log::endl;
//		bins[0].mSimpleBins.print(1, true, true, true, true);
//		//Log::out << "send.mCuckooBins.print(true, false, false);" << Log::endl;
//		//send.mCuckooBins.print(1,true, true, false);
//
//
//
//
//		//send.mBins.print();
//
//		//for (u64 i = 1; i < 3; ++i)
//		//{
//		//	Log::out << "Sender Bin#: " << i << " ";
//		//	for (u64 j = 1; j < send.mBins.mBins[i].mIdx.size(); ++j)
//		//	{
//		//		Log::out << send.mBins.mBins[i].mIdx[j] << " - ";
//		//		Log::out << send.mBins.mBins[i].mValOPRF[j] << Log::endl;
//		//	}
//		//}
//		//
//		//
//
//	});
//
//	bins[1].init(1, numParties, setSize, psiSecParam,opt);
//	u64 otCountRecv = bins[1].mCuckooBins.mBins.size();
//
//	recv[0].init(opt, numParties, setSize, psiSecParam, bitSize, recvChl, otCountRecv, otRecv[1], otSend[1], ZeroBlock);
//
//	bins[1].hashing2Bins(recvSet, 2);
//
//	//recv.hash2Bins(recvSet, recvChl);
//	recv[0].getOPRFkeys(0, bins[1], recvChl);
//	recv[0].recvSS(0, bins[1], recvPayLoads, recvChl);
//	//recv.sendSS(0, sendPayLoads, recvChl);
//
//	Log::out << "recv.mCuckooBins.print(true, false, false);" << Log::endl;
//	bins[1].mCuckooBins.print(0, true, true, true);
//
//	//Log::out << "recv.mSimpleBins.print(true, false, false,false);" << Log::endl;
//	//recv.mSimpleBins.print(0,true, true, true, true);
//
//	/*recv.sendInput(recvSet, recvChl);
//	recv.mBins.print();
//	*/
//
//#ifdef PRINT
//	std::cout << IoStream::lock;
//	for (u64 i = 1; i < recvPayLoads.size(); ++i)
//	{
//		Log::out << recvPayLoads[i] << Log::endl;
//		Log::out << sendPayLoads[i] << Log::endl;
//		if (memcmp((u8*)&recvPayLoads[i], &sendPayLoads[i], sizeof(block)))
//		{
//			Log::out << "recvPayLoads[i] != sendPayLoads[i]" << Log::endl;
//			Log::out << recvSet[i] << Log::endl;
//			Log::out << sendSet[i] << Log::endl;
//			Log::out << i << Log::endl;
//		}
//
//	}
//
//	std::cout << IoStream::unlock;
//
//	std::cout << IoStream::lock;
//	Log::out << otSend[0].mT.size()[1] << Log::endl;
//	Log::out << otSend[1].mT.size()[1] << Log::endl;
//	Log::out << otSend[0].mGens[0].get<block>() << Log::endl;
//	Log::out << otRecv[0].mGens[0][0].get<block>() << Log::endl;
//	Log::out << otRecv[0].mGens[0][1].get<block>() << Log::endl;
//	Log::out << "------------" << Log::endl;
//	Log::out << otSend[1].mGens[0].get<block>() << Log::endl;
//	Log::out << otRecv[1].mGens[0][0].get<block>() << Log::endl;
//	Log::out << otRecv[1].mGens[0][1].get<block>() << Log::endl;
//	std::cout << IoStream::unlock;
//
//#endif
//
//	thrd.join();
//
//
//
//
//
//
//
//	sendChl[0]->close();
//	recvChl[0]->close();
//
//	ep0.stop();
//	ep1.stop();
//	ios.stop();
}

std::vector<block> mSet;
u64 nParties(4);
void testShareValue()
{
	u64 setSize = 5; u64 myIdx = 1;
	PRNG prng(_mm_set_epi32(4253465, 3434565, 1, 1));
	std::vector<std::vector<block>> sendPayLoads(nParties);

	for (u64 idxP = 0; idxP < nParties; ++idxP)
	{
		sendPayLoads[idxP].resize(setSize);
		for (u64 i = 0; i < setSize; ++i)
			sendPayLoads[idxP][i] = prng.get<block>();
	}
	u64 nextNeighbor = 2;// (myIdx + 1) % nParties;
	u64 prevNeighbor = (myIdx - 1 + nParties) % nParties;
	//sum share of other party =0 => compute the share to his neighbor = sum of other shares
	for (u64 i = 0; i < setSize; ++i)
	{
		block sum = ZeroBlock;
		//sendPayLoads[nextNeighbor][i] = ZeroBlock;
		for (u64 idxP = 0; idxP < nParties; ++idxP)
		{
			if ((idxP != myIdx && idxP != nextNeighbor))
				sum = sum ^ sendPayLoads[idxP][i];
		}
		std::cout << "sum: " << sum << std::endl;
		sendPayLoads[nextNeighbor][i] = sum;

		block check = ZeroBlock;

		for (u64 idxP = 0; idxP < nParties; ++idxP)
		{
			if (idxP != myIdx)
				check = check ^ sendPayLoads[idxP][i];
		}
		std::cout << "check: " << check << std::endl;

		block check2 = ZeroBlock;
		check2 = sendPayLoads[0][i] ^ sendPayLoads[3][i];
		std::cout << "check2: " << check2 << std::endl;

	}


}
void party(u64 myIdx, u64 setSize, std::vector<block>& mSet)
{
	u64  psiSecParam = 40, bitSize = 128, numThreads = 1;
	PRNG prng(_mm_set_epi32(4253465, 3434565, myIdx, myIdx));
	u64 maskSize = roundUpTo(psiSecParam + 2 * std::log2(setSize) - 1, 8) / 8;

	std::vector<block> set(setSize);
	std::vector<std::vector<block>> sendPayLoads(nParties), recvPayLoads(nParties);

	for (u64 i = 0; i < setSize; ++i)
	{
		set[i] = mSet[i];
	}
	PRNG prng1(_mm_set_epi32(4253465, 3434565, 234435, myIdx));
	//set[0] = prng1.get<block>();;
	for (u64 idxP = 0; idxP < nParties; ++idxP)
	{
		sendPayLoads[idxP].resize(setSize);
		recvPayLoads[idxP].resize(setSize);
		for (u64 i = 0; i < setSize; ++i)
			sendPayLoads[idxP][i] = prng.get<block>();
	}
	u64 nextNeighbor = (myIdx + 1) % nParties;
	u64 prevNeighbor = (myIdx - 1 + nParties) % nParties;
	//sum share of other party =0 => compute the share to his neighbor = sum of other shares
	if (myIdx != 0) {
		for (u64 i = 0; i < setSize; ++i)
		{
			block sum = ZeroBlock;
			for (u64 idxP = 0; idxP < nParties; ++idxP)
			{
				if ((idxP != myIdx && idxP != nextNeighbor))
					sum = sum ^ sendPayLoads[idxP][i];
			}
			sendPayLoads[nextNeighbor][i] = sum;

		}
	}
	else
		for (u64 i = 0; i < setSize; ++i)
		{
			sendPayLoads[myIdx][i] = ZeroBlock;
			for (u64 idxP = 0; idxP < nParties; ++idxP)
			{
				if (idxP != myIdx)
					sendPayLoads[myIdx][i] = sendPayLoads[myIdx][i] ^ sendPayLoads[idxP][i];
			}
		}

#ifdef PRINT
	std::cout << IoStream::lock;
	if (myIdx != 0) {
		for (u64 i = 0; i < setSize; ++i)
		{
			block check = ZeroBlock;
			for (u64 idxP = 0; idxP < nParties; ++idxP)
			{
				if (idxP != myIdx)
					check = check ^ sendPayLoads[idxP][i];
			}
			if (memcmp((u8*)&check, &ZeroBlock, sizeof(block)))
				std::cout << "Error ss values: myIdx: " << myIdx
				<< " value: " << check << std::endl;
		}
	}
	else
		for (u64 i = 0; i < setSize; ++i)
		{
			block check = ZeroBlock;
			for (u64 idxP = 0; idxP < nParties; ++idxP)
			{
				check = check ^ sendPayLoads[idxP][i];
			}
			if (memcmp((u8*)&check, &ZeroBlock, sizeof(block)))
				std::cout << "Error ss values: myIdx: " << myIdx
				<< " value: " << check << std::endl;
		}
	std::cout << IoStream::unlock;
#endif

	std::string name("psi");
	BtIOService ios(0);

	int btCount = nParties;
	std::vector<BtEndpoint> ep(nParties);

	for (u64 i = 0; i < nParties; ++i)
	{
		if (i < myIdx)
		{
			u32 port = i * 10 + myIdx;//get the same port; i=1 & pIdx=2 =>port=102
			ep[i].start(ios, "localhost", port, false, name); //channel bwt i and pIdx, where i is sender
		}
		else if (i > myIdx)
		{
			u32 port = myIdx * 10 + i;//get the same port; i=2 & pIdx=1 =>port=102
			ep[i].start(ios, "localhost", port, true, name); //channel bwt i and pIdx, where i is receiver
		}
	}


	std::vector<std::vector<Channel*>> chls(nParties);

	for (u64 i = 0; i < nParties; ++i)
	{
		if (i != myIdx) {
			chls[i].resize(numThreads);
			for (u64 j = 0; j < numThreads; ++j)
			{
				//chls[i][j] = &ep[i].addChannel("chl" + std::to_string(j), "chl" + std::to_string(j));
				chls[i][j] = &ep[i].addChannel(name, name);
			}
		}
	}

	std::vector<KkrtNcoOtReceiver> otRecv(nParties);
	std::vector<KkrtNcoOtSender> otSend(nParties);

	std::vector<OPPRFSender> send(nParties - myIdx - 1);
	std::vector<OPPRFReceiver> recv(myIdx);
	binSet bins;

	std::vector<std::thread>  pThrds(nParties);

	//##########################
	//### Offline Phasing
	//##########################
	Timer timer;
	auto start = timer.setTimePoint("start");
	bins.init(myIdx, nParties, setSize, psiSecParam,opt);
	u64 otCountSend = bins.mSimpleBins.mBins.size();
	u64 otCountRecv = bins.mCuckooBins.mBins.size();

	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
	{
		pThrds[pIdx] = std::thread([&, pIdx]() {
			if (pIdx < myIdx) {
				//I am a receiver if other party idx < mine
				recv[pIdx].init(opt, nParties, setSize, psiSecParam, bitSize, chls[pIdx], otCountRecv, otRecv[pIdx], otSend[pIdx], ZeroBlock, true);
			}
			else if (pIdx > myIdx) {
				send[pIdx - myIdx - 1].init(opt, nParties, setSize, psiSecParam, bitSize, chls[pIdx], otCountSend, otSend[pIdx], otRecv[pIdx], prng.get<block>(), true);
			}
		});
	}

	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
		pThrds[pIdx].join();

	auto initDone = timer.setTimePoint("initDone");

#ifdef PRINT
	std::cout << IoStream::lock;
	if (myIdx == 0)
	{
		Log::out << otSend[2].mGens[0].get<block>() << Log::endl;
		if (otRecv[2].hasBaseOts())
		{
			Log::out << otRecv[2].mGens[0][0].get<block>() << Log::endl;
			Log::out << otRecv[2].mGens[0][1].get<block>() << Log::endl;
		}
		Log::out << "------------" << Log::endl;
	}
	if (myIdx == 2)
	{
		if (otSend[0].hasBaseOts())
			Log::out << otSend[0].mGens[0].get<block>() << Log::endl;

		Log::out << otRecv[0].mGens[0][0].get<block>() << Log::endl;
		Log::out << otRecv[0].mGens[0][1].get<block>() << Log::endl;
	}
	std::cout << IoStream::unlock;
#endif

	//##########################
	//### Hashing
	//##########################
	bins.hashing2Bins(set, 1);

	//if(myIdx==0)
	//	bins.mSimpleBins.print(myIdx, true, false, false, false);
	//if (myIdx == 2)
	//	bins.mCuckooBins.print(myIdx, true, false, false);

	auto hashingDone = timer.setTimePoint("hashingDone");
	//##########################
	//### Online Phasing - compute OPRF
	//##########################

	pThrds.clear();
	pThrds.resize(nParties);
	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
	{
		pThrds[pIdx] = std::thread([&, pIdx]() {
			if (pIdx < myIdx) {
				//I am a receiver if other party idx < mine
				recv[pIdx].getOPRFkeys(pIdx, bins, chls[pIdx], true);
			}
			else if (pIdx > myIdx) {

				send[pIdx - myIdx - 1].getOPRFkeys(pIdx, bins, chls[pIdx], true);
			}
		});
	}

	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
		pThrds[pIdx].join();

	//if (myIdx == 0)
	//{
	//	bins.mSimpleBins.print(2, true, true, false, false);
	//	//bins.mCuckooBins.print(2, true, false, false);
	//	Log::out << "------------" << Log::endl;
	//}
	//if (myIdx == 2)
	//{
	//	//bins.mSimpleBins.print(myIdx, true, false, false, false);
	//	bins.mCuckooBins.print(0, true, true, false);
	//}

	auto getOPRFDone = timer.setTimePoint("getOPRFDone");

	//##########################
	//### online phasing - secretsharing
	//##########################
	pThrds.clear();
	pThrds.resize(nParties);

	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
	{
		pThrds[pIdx] = std::thread([&, pIdx]() {
			if ((pIdx < myIdx && pIdx != prevNeighbor)) {
				//I am a receiver if other party idx < mine				
				recv[pIdx].recvSS(pIdx, bins, recvPayLoads[pIdx], chls[pIdx]);
				recv[pIdx].sendSS(pIdx, bins, sendPayLoads[pIdx], chls[pIdx]);
			}
			else if (pIdx > myIdx && pIdx != nextNeighbor) {
				send[pIdx - myIdx - 1].sendSS(pIdx, bins, sendPayLoads[pIdx], chls[pIdx]);
				send[pIdx - myIdx - 1].recvSS(pIdx, bins, recvPayLoads[pIdx], chls[pIdx]);
			}

			else if (pIdx == prevNeighbor && myIdx != 0) {
				recv[pIdx].sendSS(pIdx, bins, sendPayLoads[pIdx], { chls[pIdx] });
			}
			else if (pIdx == nextNeighbor && myIdx != nParties - 1)
			{
				send[pIdx - myIdx - 1].recvSS(pIdx, bins, recvPayLoads[pIdx], chls[pIdx]);
			}

			else if (pIdx == nParties - 1 && myIdx == 0) {
				send[pIdx - myIdx - 1].sendSS(pIdx, bins, sendPayLoads[pIdx], chls[pIdx]);
			}

			else if (pIdx == 0 && myIdx == nParties - 1)
			{
				recv[pIdx].recvSS(pIdx, bins, recvPayLoads[pIdx], chls[pIdx]);
			}

		});
	}

	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
		pThrds[pIdx].join();

	auto getSSDone2Dir = timer.setTimePoint("getSSDone2Dir");

#ifdef PRINT
	std::cout << IoStream::lock;
	if (myIdx == 0)
	{
		for (int i = 0; i < 3; i++)
		{
			block temp = ZeroBlock;
			memcpy((u8*)&temp, (u8*)&sendPayLoads[2][i], maskSize);
			Log::out << "s " << myIdx << " - 2: Idx" << i << " - " << temp << Log::endl;

			block temp1 = ZeroBlock;
			memcpy((u8*)&temp1, (u8*)&recvPayLoads[2][i], maskSize);
			Log::out << "r " << myIdx << " - 2: Idx" << i << " - " << temp1 << Log::endl;
		}
		Log::out << "------------" << Log::endl;
	}
	if (myIdx == 2)
	{
		for (int i = 0; i < 3; i++)
		{
			block temp = ZeroBlock;
			memcpy((u8*)&temp, (u8*)&recvPayLoads[0][i], maskSize);
			Log::out << "r " << myIdx << " - 0: Idx" << i << " - " << temp << Log::endl;

			block temp1 = ZeroBlock;
			memcpy((u8*)&temp1, (u8*)&sendPayLoads[0][i], maskSize);
			Log::out << "s " << myIdx << " - 0: Idx" << i << " - " << temp1 << Log::endl;
		}
		Log::out << "------------" << Log::endl;
	}
	std::cout << IoStream::unlock;
#endif
	//##########################
	//### online phasing - secretsharing - round
	//##########################

	if (myIdx == 0)
	{
		// Xor the received shares
		for (u64 i = 0; i < setSize; ++i)
		{
			for (u64 idxP = 0; idxP < nParties; ++idxP)
			{
				if (idxP != myIdx && idxP != prevNeighbor)
					sendPayLoads[nextNeighbor][i] = sendPayLoads[nextNeighbor][i] ^ recvPayLoads[idxP][i];
			}
		}

		send[nextNeighbor].sendSS(nextNeighbor, bins, sendPayLoads[nextNeighbor], chls[nextNeighbor]);
		send[nextNeighbor - myIdx - 1].recvSS(prevNeighbor, bins, recvPayLoads[prevNeighbor], chls[prevNeighbor]);

	}
	else if (myIdx == nParties - 1)
	{
		recv[prevNeighbor].recvSS(prevNeighbor, bins, recvPayLoads[prevNeighbor], chls[prevNeighbor]);

		//Xor the received shares 	
		for (u64 i = 0; i < setSize; ++i)
		{
			sendPayLoads[nextNeighbor][i] = sendPayLoads[nextNeighbor][i] ^ recvPayLoads[prevNeighbor][i];
			for (u64 idxP = 0; idxP < nParties; ++idxP)
			{
				if (idxP != myIdx && idxP != prevNeighbor)
					sendPayLoads[nextNeighbor][i] = sendPayLoads[nextNeighbor][i] ^ recvPayLoads[idxP][i];
			}
		}

		recv[nextNeighbor].sendSS(nextNeighbor, bins, sendPayLoads[nextNeighbor], chls[nextNeighbor]);

	}
	else
	{
		recv[prevNeighbor].recvSS(prevNeighbor, bins, recvPayLoads[prevNeighbor], chls[prevNeighbor]);
		//Xor the received shares 	
		for (u64 i = 0; i < setSize; ++i)
		{
			sendPayLoads[nextNeighbor][i] = sendPayLoads[nextNeighbor][i] ^ recvPayLoads[prevNeighbor][i];
			for (u64 idxP = 0; idxP < nParties; ++idxP)
			{
				if (idxP != myIdx && idxP != prevNeighbor)
					sendPayLoads[nextNeighbor][i] = sendPayLoads[nextNeighbor][i] ^ recvPayLoads[idxP][i];
			}
		}
		send[nextNeighbor - myIdx - 1].sendSS(nextNeighbor, bins, sendPayLoads[nextNeighbor], chls[nextNeighbor]);
	}

	auto getSSDoneRound = timer.setTimePoint("getSSDoneRound");


#ifdef PRINT
	std::cout << IoStream::lock;
	if (myIdx == 0)
	{
		for (int i = 0; i < 5; i++)
		{
			block temp = ZeroBlock;
			memcpy((u8*)&temp, (u8*)&sendPayLoads[1][i], maskSize);
			Log::out << myIdx << " - " << temp << Log::endl;
			//Log::out << recvPayLoads[2][i] << Log::endl;
		}
		Log::out << "------------" << Log::endl;
	}
	if (myIdx == 1)
	{
		for (int i = 0; i < 5; i++)
		{
			block temp = ZeroBlock;
			memcpy((u8*)&temp, (u8*)&recvPayLoads[0][i], maskSize);
			Log::out << myIdx << " - " << temp << Log::endl;
			//Log::out << sendPayLoads[0][i] << Log::endl;
		}
	}
	std::cout << IoStream::unlock;
#endif

	//##########################
	//### online phasing - compute intersection
	//##########################

	if (myIdx == 0) {
		std::vector<u64> mIntersection;
		u64 maskSize = roundUpTo(psiSecParam + 2 * std::log2(setSize) - 1, 8) / 8;
		for (u64 i = 0; i < setSize; ++i)
		{
			if (!memcmp((u8*)&sendPayLoads[myIdx][i], &recvPayLoads[prevNeighbor][i], maskSize))
			{
				mIntersection.push_back(i);
			}
		}
		Log::out << "mIntersection.size(): " << mIntersection.size() << Log::endl;
	}
	auto getIntersection = timer.setTimePoint("getIntersection");


	if (myIdx == 0) {
		auto offlineTime = std::chrono::duration_cast<std::chrono::milliseconds>(initDone - start).count();
		auto hashingTime = std::chrono::duration_cast<std::chrono::milliseconds>(hashingDone - initDone).count();
		auto getOPRFTime = std::chrono::duration_cast<std::chrono::milliseconds>(getOPRFDone - hashingDone).count();
		auto ss2DirTime = std::chrono::duration_cast<std::chrono::milliseconds>(getSSDone2Dir - getOPRFDone).count();
		auto ssRoundTime = std::chrono::duration_cast<std::chrono::milliseconds>(getSSDoneRound - getSSDone2Dir).count();
		auto intersectionTime = std::chrono::duration_cast<std::chrono::milliseconds>(getIntersection - getSSDoneRound).count();

		double onlineTime = hashingTime + getOPRFTime + ss2DirTime + ssRoundTime + intersectionTime;

		double time = offlineTime + onlineTime;
		time /= 1000;

		std::cout << "setSize: " << setSize << "\n"
			<< "offlineTime:  " << offlineTime << " ms\n"
			<< "hashingTime:  " << hashingTime << " ms\n"
			<< "getOPRFTime:  " << getOPRFTime << " ms\n"
			<< "ss2DirTime:  " << ss2DirTime << " ms\n"
			<< "ssRoundTime:  " << ssRoundTime << " ms\n"
			<< "intersection:  " << intersectionTime << " ms\n"
			<< "onlineTime:  " << onlineTime << " ms\n"
			<< "Total time: " << time << " s\n"
			<< "------------------\n";

	}



	for (u64 i = 0; i < nParties; ++i)
	{
		if (i != myIdx)
		{
			for (u64 j = 0; j < numThreads; ++j)
			{
				chls[i][j]->close();
			}
		}
	}

	for (u64 i = 0; i < nParties; ++i)
	{
		if (i != myIdx)
			ep[i].stop();
	}


	ios.stop();
}

//3-party PSI =>1 way direction: only P0 creates a secret sharing 'sendPayLoads'
//1. run OPPRF btw P0 and P1 where P1 is receiver
//2. run OPPRF btw P1 and P2 where P1 is sender with the payload received from P0
//3. run OPPRF btw P2 and P0 where P2 is sender with the payload received from P1
//4. P0 compare the received payload and his own secret sharing 'sendPayLoads'. 
// If it is equal to 0 => intersection.

void party2(u64 myIdx, u64 setSize, std::vector<block>& mSet)
{
	
	nParties = 2;
	u64 psiSecParam = 40, bitSize = 128, numThreads = 1;
	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));

	std::vector<block> set(setSize);
	for (u64 i = 0; i < setSize; ++i)
		set[i] = mSet[i];

	PRNG prng1(_mm_set_epi32(4253465, 3434565, 234435, myIdx)); //for test
	set[0] = prng1.get<block>();;

	std::vector<block> sendPayLoads, recvPayLoads;
	if (myIdx == 1)
		 sendPayLoads.resize(setSize);
	if (myIdx == 0)
		recvPayLoads.resize(setSize);
	
	//only P0 genaretes secret sharing
	if (myIdx == 1)
	{
		for (u64 i = 0; i < setSize; ++i)
		{
			sendPayLoads[i] = prng.get<block>();
		}
	}


	std::string name("psi");
	BtIOService ios(0);

	int btCount = nParties;
	std::vector<BtEndpoint> ep(nParties);

	for (u64 i = 0; i < nParties; ++i)
	{
		if (i < myIdx)
		{
			u32 port = i * 10 + myIdx;//get the same port; i=1 & pIdx=2 =>port=102
			ep[i].start(ios, "localhost", port, false, name); //channel bwt i and pIdx, where i is sender
		}
		else if (i > myIdx)
		{
			u32 port = myIdx * 10 + i;//get the same port; i=2 & pIdx=1 =>port=102
			ep[i].start(ios, "localhost", port, true, name); //channel bwt i and pIdx, where i is receiver
		}
	}


	std::vector<std::vector<Channel*>> chls(nParties);

	for (u64 i = 0; i < nParties; ++i)
	{
		if (i != myIdx) {
			chls[i].resize(numThreads);
			for (u64 j = 0; j < numThreads; ++j)
			{
				//chls[i][j] = &ep[i].addChannel("chl" + std::to_string(j), "chl" + std::to_string(j));
				chls[i][j] = &ep[i].addChannel(name, name);
			}
		}
	}

	std::vector<KkrtNcoOtReceiver> otRecv(nParties);
	std::vector<KkrtNcoOtSender> otSend(nParties);

	OPPRFSender send;
	OPPRFReceiver recv;
	binSet bins;

	std::vector<std::thread>  pThrds(nParties);

	//##########################
	//### Offline Phasing
	//##########################

	bins.init(myIdx, nParties, setSize, psiSecParam,opt);
	u64 otCountSend = bins.mSimpleBins.mBins.size();
	u64 otCountRecv = bins.mCuckooBins.mBins.size();

			if (myIdx == 1) {
				//I am a sender to my next neigbour
				send.init(opt, nParties, setSize, psiSecParam, bitSize, chls[0], otCountSend, otSend[0], otRecv[0], prng.get<block>(), false);
				//send.testSender();
			}
			else if (myIdx == 0) {
				//I am a recv to my previous neigbour
				recv.init(opt, nParties, setSize, psiSecParam, bitSize, chls[1], otCountRecv, otRecv[1], otSend[1], ZeroBlock, false);
				//recv.testReceiver();
			}




	std::cout << IoStream::lock;
	if (myIdx == 0)
	{
		Log::out << "------0------" << Log::endl;
		Log::out << otRecv[1].mGens[0][0].get<block>() << Log::endl;
		Log::out << otRecv[1].mGens[0][1].get<block>() << Log::endl;
	}
	if (myIdx == 1)
	{
		Log::out << "------1------" << Log::endl;
		Log::out << otSend[0].mGens[0].get<block>() << Log::endl;
	}

	std::cout << IoStream::unlock;



	//##########################
	//### Hashing
	//##########################
	bins.hashing2Bins(set, 1);
	//bins.mSimpleBins.print(myIdx, true, false, false, false);
	//bins.mCuckooBins.print(myIdx, true, false, false);

	//##########################
	//### Online Phasing - compute OPRF
	//##########################

	pThrds.clear();
	pThrds.resize(nParties);
	
			if (myIdx == 1) {
				//I am a sender to my next neigbour
				send.getOPRFkeys(0, bins, chls[0], false);
			}
			else if (myIdx == 0) {
				//I am a recv to my previous neigbour
				recv.getOPRFkeys(1, bins, chls[1], false);
			}
	

	if (myIdx == 0)
	{
		//bins.mSimpleBins.print(2, true, true, false, false);
		bins.mCuckooBins.print(1, true, true, false);
		Log::out << "------------" << Log::endl;
	}
	if (myIdx == 1)
	{
		bins.mSimpleBins.print(0, true, true, false, false);
		//bins.mCuckooBins.print(0, true, true, false);
	}



	//##########################
	//### online phasing - secretsharing
	//##########################

	if (myIdx == 0)
	{
			recv.recvSS(1, bins, recvPayLoads, chls[1]);
	}
	else if (myIdx == 1)
	{
		send.sendSS(0, bins, sendPayLoads, chls[0]);
	}
	

	std::cout << IoStream::lock;
	if (myIdx == 0)
	{
		//u64 
		//block x0= set[bins.mCuckooBins.mBins[0].idx()];

		for (int i = 0; i < 5; i++)
		{

			Log::out << myIdx << "r-" << recvPayLoads[i] << Log::endl;
		}
		Log::out << "------------" << Log::endl;
	}
	if (myIdx == 1)
	{
		for (int i = 0; i < 5; i++)
		{
			//Log::out << recvPayLoads[i] << Log::endl;
			Log::out << myIdx << "s-" << sendPayLoads[i] << Log::endl;
		}
	}
	
	std::cout << IoStream::unlock;

#if 1
	//##########################
	//### online phasing - compute intersection
	//##########################

	if (myIdx == 0) {
		std::vector<u64> mIntersection;
		u64 maskSize = roundUpTo(psiSecParam + 2 * std::log2(setSize) - 1, 8) / 8;
		for (u64 i = 0; i < setSize; ++i)
		{
			//	if (sendPayLoads[i]== recvPayLoads[i])
		//	if (!memcmp((u8*)&sendPayLoads[i], &recvPayLoads[i], maskSize))
		//	{
		//		mIntersection.push_back(i);
		//	}
		}
		//Log::out << "mIntersection.size(): "<<mIntersection.size() << Log::endl;
	}

#endif // 0
	for (u64 i = 0; i < nParties; ++i)
	{
		if (i != myIdx)
		{
			for (u64 j = 0; j < numThreads; ++j)
			{
				chls[i][j]->close();
			}
		}
	}

	for (u64 i = 0; i < nParties; ++i)
	{
		if (i != myIdx)
			ep[i].stop();
	}


	ios.stop();
}


void aug_party(u64 myIdx, u64 nParties, u64 setSize, std::vector<block>& mSet, std::vector<PRNG>& mSeedPrng)
{
	//opt = 1;
	u64 pIdxTest = 1;
	u64 psiSecParam = 40, bitSize = 128, numThreads = 1;
	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));

	std::vector<block> set(setSize);
	for (u64 i = 0; i < setSize; ++i)
		set[i] = mSet[i];

	
	PRNG prng1(_mm_set_epi32(4253465, 3434565, 234435, myIdx)); //for test
	//set[0] = prng1.get<block>();;

	std::vector<block> sendPayLoads;
	std::vector<std::vector<block>> recvPayLoads(nParties); //leader

	sendPayLoads.resize(setSize);
	
	if (myIdx == 0) //leader
		for (u32 i = 0; i < recvPayLoads.size(); i++)
		{
			recvPayLoads[i].resize(setSize);
		}

	//only P0 genaretes secret sharing
	//if (myIdx !=0)
	{
		for (u64 i = 0; i < setSize; ++i)
		{
			sendPayLoads[i] = ZeroBlock;
			for (u64 pIdx = 0; pIdx < nParties; pIdx++)
			{
				if(pIdx!=myIdx)
					sendPayLoads[i] = sendPayLoads[i]^ mSeedPrng[pIdx].get<block>();
			}
			
		}
	}


	std::string name("psi");
	BtIOService ios(0);

	int btCount = nParties;
	std::vector<BtEndpoint> ep(nParties);

	for (u64 i = 0; i < nParties; ++i)
	{
		if (i < myIdx)
		{
			u32 port = 1120 + i * 100 + myIdx;//get the same port; i=1 & pIdx=2 =>port=102
			ep[i].start(ios, "localhost", port, false, name); //channel bwt i and pIdx, where i is sender
		}
		else if (i > myIdx)
		{
			u32 port = 1120 + myIdx * 100 + i;//get the same port; i=2 & pIdx=1 =>port=102
			ep[i].start(ios, "localhost", port, true, name); //channel bwt i and pIdx, where i is receiver
		}
	}
	


	std::vector<std::vector<Channel*>> chls(nParties);

	for (u64 i = 0; i < nParties; ++i)
	{
		if (i != myIdx) {
			chls[i].resize(numThreads);
			for (u64 j = 0; j < numThreads; ++j)
			{
				//chls[i][j] = &ep[i].addChannel("chl" + std::to_string(j), "chl" + std::to_string(j));
				chls[i][j] = &ep[i].addChannel(name, name);
			}
		}
	}

	std::vector<KkrtNcoOtReceiver> otRecv(nParties);
	std::vector<KkrtNcoOtSender> otSend(nParties);

	OPPRFSender send;
	binSet bins;

	std::vector<OPPRFReceiver> recv(nParties);


	std::vector<std::thread>  pThrds(nParties);

	//##########################
	//### Offline Phasing
	//##########################

	bins.init(myIdx, nParties, setSize, psiSecParam,opt);
	

	u64 otCountSend = bins.mSimpleBins.mBins.size();
	u64 otCountRecv = bins.mCuckooBins.mBins.size();





	if (myIdx != 0) {
		//I am a sender to my next neigbour
		send.init(opt, nParties, setSize, psiSecParam, bitSize, chls[0], otCountSend, otSend[0], otRecv[0], prng.get<block>(), false);
		//send.testSender();
	}
	else if (myIdx == 0) {

		std::vector<std::thread>  pThrds(nParties);

		for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
		{
			pThrds[pIdx] = std::thread([&, pIdx]() {
				if (pIdx != 0)
				{
					recv[pIdx].init(opt, nParties, setSize, psiSecParam, bitSize, chls[pIdx], otCountRecv, otRecv[pIdx], otSend[pIdx], ZeroBlock, false);
				}
			});
		}
		for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
			pThrds[pIdx].join();
		//I am a recv to my previous neigbour
		
		//recv.testReceiver();
	}




	std::cout << IoStream::lock;
	if (myIdx == 0)
	{
		Log::out << "------0------" << Log::endl;
		Log::out << otRecv[pIdxTest].mGens[0][0].get<block>() << Log::endl;
		Log::out << otRecv[pIdxTest].mGens[0][1].get<block>() << Log::endl;
	}
	if (myIdx == pIdxTest)
	{
		Log::out << "------"<< pIdxTest<<"------" << Log::endl;
		Log::out << otSend[0].mGens[0].get<block>() << Log::endl;
	}

	std::cout << IoStream::unlock;



	//##########################
	//### Hashing
	//##########################
	bins.hashing2Bins(set, 1);
	//bins.mSimpleBins.print(myIdx, true, false, false, false);
	//bins.mCuckooBins.print(myIdx, true, false, false);

	//##########################
	//### Online Phasing - compute OPRF
	//##########################

	

	if (myIdx == 0) {
		//I am a sender to my next neigbour
		std::vector<std::thread>  pThrds(nParties);

		for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
		{
			pThrds[pIdx] = std::thread([&, pIdx]() {
				if(pIdx!=0)
				recv[pIdx].getOPRFkeys(pIdx, bins, chls[pIdx], false);
			});
		}
		for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
			pThrds[pIdx].join();	
	}
	else {
		//I am a recv to my previous neigbour	
		send.getOPRFkeys(0, bins, chls[0], false);
	}


	//if (myIdx == 0)
	//{
	//	//bins.mSimpleBins.print(2, true, true, false, false);
	//	//bins.mCuckooBins.print(1, true, true, false);
	//	Log::out << "------------" << Log::endl;
	//}
	//if (myIdx == 1)
	//{
	//	bins.mSimpleBins.print(0, true, true, false, false,opt);
	//	//bins.mCuckooBins.print(0, true, true, false);
	//}



	//##########################
	//### online phasing - secretsharing
	//##########################

	if (myIdx == 0)
	{
		//I am a sender to my next neigbour
		if (opt == 0 || opt == 3)
		{
			std::vector<std::thread>  pThrds(nParties);
			for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
			{

				pThrds[pIdx] = std::thread([&, pIdx]() {
					if (pIdx != 0)
						recv[pIdx].recvSS(pIdx, bins, recvPayLoads[pIdx], chls[pIdx]);
				});
			}
			for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
				pThrds[pIdx].join();
		}
		else //since NTL does not support thread safe => running in pipeline
		{
			for (u64 pIdx = 0; pIdx < nParties; ++pIdx)
			{
				if (pIdx != 0)
					recv[pIdx].recvSS(pIdx, bins, recvPayLoads[pIdx], chls[pIdx]);
			}
		}

		
	}
	else 
	{
		send.sendSS(0, bins, sendPayLoads, chls[0]);
	}


	std::cout << IoStream::lock;
	if (myIdx == 0)
	{
		//u64 
		//block x0= set[bins.mCuckooBins.mBins[0].idx()];

		//for (int i = 0; i < 5; i++)
		{

			Log::out << myIdx << "r-5" << recvPayLoads[pIdxTest][5] << Log::endl;
			Log::out << myIdx << "r-4" << recvPayLoads[pIdxTest][4] << Log::endl;
			Log::out << myIdx << "r-13" << recvPayLoads[pIdxTest][13] << Log::endl;
		}
		Log::out << "------------" << Log::endl;
	}
	if (myIdx == pIdxTest)
	{
		//for (int i = 0; i < 5; i++)
		{
			//Log::out << recvPayLoads[i] << Log::endl;
			Log::out << myIdx << "s-5" << sendPayLoads[5] << Log::endl;
			Log::out << myIdx << "s-4" << sendPayLoads[4] << Log::endl;
			Log::out << myIdx << "s-13" << sendPayLoads[13] << Log::endl;
		}
	}

	std::cout << IoStream::unlock;

#if 0
#endif // 0
#if 1
	//##########################
	//### online phasing - compute intersection
	//##########################

	if (myIdx == 0) {
		std::vector<u64> mIntersection;
		
		for (u64 i = 0; i < setSize; ++i)
		{
			block sum = sendPayLoads[i];
			for (u64 pIdx = 0; pIdx < nParties; pIdx++)
			{
				if (pIdx != myIdx)
				{
					//sum = sum ^ mSeedPrng[pIdx].get<block>();
					sum = sum^recvPayLoads[pIdx][i];
				}
			}
			//std::cout << sum << std::endl;
							
				if (!memcmp((u8*)&sum, (u8*)&ZeroBlock, bins.mMaskSize))
				{
					mIntersection.push_back(i);
				}
		}
		Log::out << "mIntersection.size(): " << mIntersection.size() << Log::endl;
	}

#endif // 0
	for (u64 i = 0; i < nParties; ++i)
	{
		if (i != myIdx)
		{
			for (u64 j = 0; j < numThreads; ++j)
			{
				chls[i][j]->close();
			}
		}
	}

	for (u64 i = 0; i < nParties; ++i)
	{
		if (i != myIdx)
			ep[i].stop();
	}


	ios.stop();
}

void party3(u64 myIdx, u64 setSize, std::vector<block>& mSet)
{
	nParties = 3;
	u64 psiSecParam = 40, bitSize = 128, numThreads = 1;
	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));

	std::vector<block> set(setSize);
	for (u64 i = 0; i < setSize; ++i)
		set[i] = mSet[i];

	PRNG prng1(_mm_set_epi32(4253465, 3434565, 234435, myIdx)); //for test
	set[0] = prng1.get<block>();;

	std::vector<block> sendPayLoads(setSize);
	std::vector<block> recvPayLoads(setSize);

	//only P0 genaretes secret sharing
	if (myIdx == 0)
	{
		for (u64 i = 0; i < setSize; ++i)
			sendPayLoads[i] = prng.get<block>();
	}


	std::string name("psi");
	BtIOService ios(0);

	int btCount = nParties;
	std::vector<BtEndpoint> ep(nParties);

	for (u64 i = 0; i < nParties; ++i)
	{
		if (i < myIdx)
		{
			u32 port = i * 10 + myIdx;//get the same port; i=1 & pIdx=2 =>port=102
			ep[i].start(ios, "localhost", port, false, name); //channel bwt i and pIdx, where i is sender
		}
		else if (i > myIdx)
		{
			u32 port = myIdx * 10 + i;//get the same port; i=2 & pIdx=1 =>port=102
			ep[i].start(ios, "localhost", port, true, name); //channel bwt i and pIdx, where i is receiver
		}
	}


	std::vector<std::vector<Channel*>> chls(nParties);

	for (u64 i = 0; i < nParties; ++i)
	{
		if (i != myIdx) {
			chls[i].resize(numThreads);
			for (u64 j = 0; j < numThreads; ++j)
			{
				//chls[i][j] = &ep[i].addChannel("chl" + std::to_string(j), "chl" + std::to_string(j));
				chls[i][j] = &ep[i].addChannel(name, name);
			}
		}
	}

	std::vector<KkrtNcoOtReceiver> otRecv(nParties);
	std::vector<KkrtNcoOtSender> otSend(nParties);

	OPPRFSender send;
	OPPRFReceiver recv;
	binSet bins;

	std::vector<std::thread>  pThrds(nParties);

	//##########################
	//### Offline Phasing
	//##########################

	bins.init(myIdx, nParties, setSize, psiSecParam,opt);
	u64 otCountSend = bins.mSimpleBins.mBins.size();
	u64 otCountRecv = bins.mCuckooBins.mBins.size();

	u64 nextNeibough = (myIdx + 1) % nParties;
	u64 prevNeibough = (myIdx - 1 + nParties) % nParties;

	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
	{
		pThrds[pIdx] = std::thread([&, pIdx]() {
			if (pIdx == nextNeibough) {
				//I am a sender to my next neigbour
				send.init(opt, nParties, setSize, psiSecParam, bitSize, chls[pIdx], otCountSend, otSend[pIdx], otRecv[pIdx], prng.get<block>(), false);
				//send.testSender();
			}
			else if (pIdx == prevNeibough) {
				//I am a recv to my previous neigbour
				recv.init(opt, nParties, setSize, psiSecParam, bitSize, chls[pIdx], otCountRecv, otRecv[pIdx], otSend[pIdx], ZeroBlock, false);
				//recv.testReceiver();
			}
		});
	}

	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
		pThrds[pIdx].join();


	std::cout << IoStream::lock;
	if (myIdx == 0)
	{
		Log::out << "------0------" << Log::endl;
		Log::out << otSend[1].mGens[0].get<block>() << Log::endl;
		Log::out << otRecv[2].mGens[0][0].get<block>() << Log::endl;
		Log::out << otRecv[2].mGens[0][1].get<block>() << Log::endl;
	}
	if (myIdx == 1)
	{
		Log::out << "------1------" << Log::endl;
		Log::out << otRecv[0].mGens[0][0].get<block>() << Log::endl;
		Log::out << otRecv[0].mGens[0][1].get<block>() << Log::endl;
		Log::out << otSend[2].mGens[0].get<block>() << Log::endl;
	}

	if (myIdx == 2)
	{
		Log::out << "------2------" << Log::endl;
		Log::out << otRecv[1].mGens[0][0].get<block>() << Log::endl;
		Log::out << otRecv[1].mGens[0][1].get<block>() << Log::endl;
		Log::out << otSend[0].mGens[0].get<block>() << Log::endl;
	}

	std::cout << IoStream::unlock;
#if 1


	//##########################
	//### Hashing
	//##########################
	bins.hashing2Bins(set, 1);
	//bins.mSimpleBins.print(myIdx, true, false, false, false);
	//bins.mCuckooBins.print(myIdx, true, false, false);

	//##########################
	//### Online Phasing - compute OPRF
	//##########################

	pThrds.clear();
	pThrds.resize(nParties);
	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
	{
		pThrds[pIdx] = std::thread([&, pIdx]() {

			if (pIdx == nextNeibough) {
				//I am a sender to my next neigbour
				send.getOPRFkeys(pIdx, bins, chls[pIdx], false);
			}
			else if (pIdx == prevNeibough) {
				//I am a recv to my previous neigbour
				recv.getOPRFkeys(pIdx, bins, chls[pIdx], false);
			}
		});
	}

	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
		pThrds[pIdx].join();

	//if (myIdx == 2)
	//{
	//	//bins.mSimpleBins.print(2, true, true, false, false);
	//	bins.mCuckooBins.print(1, true, true, false);
	//	Log::out << "------------" << Log::endl;
	//}
	//if (myIdx == 1)
	//{
	//	bins.mSimpleBins.print(2, true, true, false, false);
	//	//bins.mCuckooBins.print(0, true, true, false);
	//}



	//##########################
	//### online phasing - secretsharing
	//##########################
	if (myIdx == 0)
	{
		send.sendSS(nextNeibough, bins, sendPayLoads, chls[nextNeibough]);
		recv.recvSS(prevNeibough, bins, recvPayLoads, chls[prevNeibough]);

	}
	else
	{
		recv.recvSS(prevNeibough, bins, recvPayLoads, chls[prevNeibough]);
		//sendPayLoads = recvPayLoads;
		send.sendSS(nextNeibough, bins, recvPayLoads, chls[nextNeibough]);
	}

	std::cout << IoStream::lock;
	if (myIdx == 0)
	{
		for (int i = 0; i < 5; i++)
		{
			Log::out << sendPayLoads[i] << Log::endl;
			//Log::out << recvPayLoads[2][i] << Log::endl;
		}
		Log::out << "------------" << Log::endl;
	}
	if (myIdx == 1)
	{
		for (int i = 0; i < 5; i++)
		{
			//Log::out << recvPayLoads[i] << Log::endl;
			Log::out << sendPayLoads[i] << Log::endl;
		}
	}
	if (myIdx == 2)
	{
		for (int i = 0; i < 5; i++)
		{
			Log::out << sendPayLoads[i] << Log::endl;
		}
	}
	std::cout << IoStream::unlock;
	//##########################
	//### online phasing - compute intersection
	//##########################

	if (myIdx == 0) {
		std::vector<u64> mIntersection;
		u64 maskSize = roundUpTo(psiSecParam + 2 * std::log2(setSize) - 1, 8) / 8;
		for (u64 i = 0; i < setSize; ++i)
		{
			//	if (sendPayLoads[i]== recvPayLoads[i])
			if (!memcmp((u8*)&sendPayLoads[i], &recvPayLoads[i], maskSize))
			{
				mIntersection.push_back(i);
			}
		}
		Log::out << mIntersection.size() << Log::endl;
	}

#endif // 0
	for (u64 i = 0; i < nParties; ++i)
	{
		if (i != myIdx)
		{
			for (u64 j = 0; j < numThreads; ++j)
			{
				chls[i][j]->close();
			}
		}
	}

	for (u64 i = 0; i < nParties; ++i)
	{
		if (i != myIdx)
			ep[i].stop();
	}


	ios.stop();
}

bool is_in_dual_area(u64 startIdx, u64 endIdx, u64 numIdx, u64 checkIdx) {
	bool res = false;
	if (startIdx <= endIdx)
	{
		if (startIdx <= checkIdx && checkIdx <= endIdx)
			res = true;
	}
	else //crosing 0, e.i, areas: startIdx....n-1, 0...endIdx
	{
		if ((0 <= checkIdx && checkIdx <= endIdx) //0...endIdx
			|| (startIdx <= checkIdx && checkIdx <= numIdx))
			//startIdx...n-1
			res = true;
	}
	return res;
}

//leader is n-1
void tparty(u64 myIdx, u64 nParties, u64 tParties, u64 setSize, std::vector<block>& mSet, u64 nTrials)
{

#pragma region setup



	//nParties = 4;
	/*std::fstream runtime;
	if (myIdx == 0)
	runtime.open("./runtime" + nParties, runtime.trunc | runtime.out);*/

	u64 leaderIdx = nParties - 1; //leader party
	u64 nSS = nParties - 1; //n-2 parties joinly operated secrete sharing
	int tSS = tParties; //ss with t next  parties, and last for leader => t+1  


	u64 offlineAvgTime(0), hashingAvgTime(0), getOPRFAvgTime(0),
		ss2DirAvgTime(0), ssRoundAvgTime(0), intersectionAvgTime(0), onlineAvgTime(0);

	u64  psiSecParam = 40, bitSize = 128, numThreads = 1;
	PRNG prng(_mm_set_epi32(4253465, 3434565, myIdx, myIdx));

	std::string name("psi");
	BtIOService ios(0);


	std::vector<BtEndpoint> ep(nParties);

	for (u64 i = 0; i < nParties; ++i)
	{
		if (i < myIdx)
		{
			u32 port = 1120 + i * 100 + myIdx;;//get the same port; i=1 & pIdx=2 =>port=102
			ep[i].start(ios, "localhost", port, false, name); //channel bwt i and pIdx, where i is sender
		}
		else if (i > myIdx)
		{
			u32 port = 1120 + myIdx * 100 + i;//get the same port; i=2 & pIdx=1 =>port=102
			ep[i].start(ios, "localhost", port, true, name); //channel bwt i and pIdx, where i is receiver
		}
	}

	std::vector<std::vector<Channel*>> chls(nParties);
	std::vector<u8> dummy(nParties);
	std::vector<u8> revDummy(nParties);

	for (u64 i = 0; i < nParties; ++i)
	{
		dummy[i] = myIdx * 10 + i;

		if (i != myIdx) {
			chls[i].resize(numThreads);
			for (u64 j = 0; j < numThreads; ++j)
			{
				//chls[i][j] = &ep[i].addChannel("chl" + std::to_string(j), "chl" + std::to_string(j));
				chls[i][j] = &ep[i].addChannel(name, name);
				//chls[i][j].mEndpoint;



			}
		}
	}


	u64 maskSize = roundUpTo(psiSecParam + 2 * std::log2(setSize) - 1, 8) / 8;
	u64 nextNeighbor = (myIdx + 1) % nParties;
	u64 prevNeighbor = (myIdx - 1 + nParties) % nParties;

#pragma endregion

	for (u64 idxTrial = 0; idxTrial < nTrials; idxTrial++)
	{
#pragma region input
		std::vector<block> set(setSize);

		std::vector<std::vector<block>>
			sendPayLoads(tParties + 1), //include the last PayLoads to leader
			recvPayLoads(tParties); //received form clients

		for (u64 i = 0; i < setSize; ++i)
		{
			set[i] = mSet[i];
		}
		PRNG prng1(_mm_set_epi32(4253465, 3434565, 234435, myIdx));
		set[0] = prng1.get<block>();;


		if (myIdx != leaderIdx) {//generate share of zero for leader myIDx!=n-1		
			for (u64 idxP = 0; idxP < tParties; ++idxP)
			{
				sendPayLoads[idxP].resize(setSize);
				for (u64 i = 0; i < setSize; ++i)
				{
					sendPayLoads[idxP][i] = prng.get<block>();
				}
			}

			sendPayLoads[tParties].resize(setSize); //share to leader at second phase
			for (u64 i = 0; i < setSize; ++i)
			{
				sendPayLoads[tParties][i] = ZeroBlock;
				for (u64 idxP = 0; idxP < tParties; ++idxP)
				{
					sendPayLoads[tParties][i] =
						sendPayLoads[tParties][i] ^ sendPayLoads[idxP][i];
				}
			}
			for (u64 idxP = 0; idxP < recvPayLoads.size(); ++idxP)
			{
				recvPayLoads[idxP].resize(setSize);
			}

		}
		else
		{
			//leader: dont send; only receive ss from clients
			sendPayLoads.resize(0);//
			recvPayLoads.resize(nParties - 1);
			for (u64 idxP = 0; idxP < recvPayLoads.size(); ++idxP)
			{
				recvPayLoads[idxP].resize(setSize);
			}

		}


#ifdef PRINT	
		std::cout << IoStream::lock;
		if (myIdx != leaderIdx) {
			for (u64 i = 0; i < setSize; ++i)
			{
				block check = ZeroBlock;
				for (u64 idxP = 0; idxP < tParties + 1; ++idxP)
				{
					//if (idxP != myIdx)
					check = check ^ sendPayLoads[idxP][i];
				}
				if (memcmp((u8*)&check, &ZeroBlock, sizeof(block)))
					std::cout << "Error ss values: myIdx: " << myIdx
					<< " value: " << check << std::endl;
			}
		}
		std::cout << IoStream::unlock;
#endif
#pragma endregion
		u64 num_threads = nParties - 1; //except P0, and my
		bool isDual = true;
		u64 idx_start_dual = 0;
		u64 idx_end_dual = 0;
		u64 t_prev_shift = tSS;

		if (myIdx != leaderIdx) {
			if (2 * tSS < nSS)
			{
				num_threads = 2 * tSS + 1;
				isDual = false;
			}
			else {
				idx_start_dual = (myIdx - tSS + nSS) % nSS;
				idx_end_dual = (myIdx + tSS) % nSS;
			}

			std::cout << IoStream::lock;
			std::cout << myIdx << "| " << idx_start_dual << " " << idx_end_dual << "\n";
			std::cout << IoStream::unlock;
		}
		std::vector<std::thread>  pThrds(num_threads);

		std::vector<KkrtNcoOtReceiver> otRecv(nParties);
		std::vector<KkrtNcoOtSender> otSend(nParties);
		std::vector<OPPRFSender> send(nParties);
		std::vector<OPPRFReceiver> recv(nParties);

		if (myIdx == leaderIdx)
		{
			/*otRecv.resize(nParties - 1);
			otSend.resize(nParties - 1);
			send.resize(nParties - 1);
			recv.resize(nParties - 1);*/
			pThrds.resize(nParties - 1);
		}



		binSet bins;

		//##########################
		//### Offline Phasing
		//##########################
		Timer timer;
		auto start = timer.setTimePoint("start");
		bins.init(myIdx, nParties, setSize, psiSecParam,opt);
		u64 otCountSend = bins.mSimpleBins.mBins.size();
		u64 otCountRecv = bins.mCuckooBins.mBins.size();


#pragma region base OT
		//##########################
		//### Base OT
		//##########################

		if (myIdx != leaderIdx)
		{
			for (u64 pIdx = 0; pIdx < tSS; ++pIdx)
			{
				u64 prevIdx = (myIdx - pIdx - 1 + nSS) % nSS;

				if (!(isDual && is_in_dual_area(idx_start_dual, idx_end_dual, nSS, prevIdx)))
				{
					u64 thr = t_prev_shift + pIdx;

					pThrds[thr] = std::thread([&, prevIdx, thr]() {

						chls[prevIdx][0]->recv(&revDummy[prevIdx], 1);

						std::cout << IoStream::lock;
						std::cout << myIdx << "| : " << "| thr[" << thr << "]:" << prevIdx << " --> " << myIdx << ": " << static_cast<int16_t>(revDummy[prevIdx]) << "\n";

						std::cout << IoStream::unlock;


						//prevIdx << " --> " << myIdx
						recv[prevIdx].init(opt, nParties, setSize, psiSecParam, bitSize, chls[prevIdx], otCountRecv, otRecv[prevIdx], otSend[prevIdx], ZeroBlock, false);

					});



				}
			}

			for (u64 pIdx = 0; pIdx < tSS; ++pIdx)
			{
				u64 nextIdx = (myIdx + pIdx + 1) % nSS;

				if ((isDual && is_in_dual_area(idx_start_dual, idx_end_dual, nSS, nextIdx))) {

					pThrds[pIdx] = std::thread([&, nextIdx, pIdx]() {


						//dual myIdx << " <-> " << nextIdx 
						if (myIdx < nextIdx)
						{
							chls[nextIdx][0]->asyncSend(&dummy[nextIdx], 1);
							std::cout << IoStream::lock;
							std::cout << myIdx << "| d: " << "| thr[" << pIdx << "]:" << myIdx << " <->> " << nextIdx << ": " << static_cast<int16_t>(dummy[nextIdx]) << "\n";
							std::cout << IoStream::unlock;

							send[nextIdx].init(opt, nParties, setSize, psiSecParam, bitSize, chls[nextIdx], otCountSend, otSend[nextIdx], otRecv[nextIdx], prng.get<block>(), true);
						}
						else if (myIdx > nextIdx) //by index
						{
							chls[nextIdx][0]->recv(&revDummy[nextIdx], 1);

							std::cout << IoStream::lock;
							std::cout << myIdx << "| d: " << "| thr[" << pIdx << "]:" << myIdx << " <<-> " << nextIdx << ": " << static_cast<int16_t>(revDummy[nextIdx]) << "\n";
							std::cout << IoStream::unlock;

							recv[nextIdx].init(opt, nParties, setSize, psiSecParam, bitSize, chls[nextIdx], otCountRecv, otRecv[nextIdx], otSend[nextIdx], ZeroBlock, true);
						}
					});

				}
				else
				{
					pThrds[pIdx] = std::thread([&, nextIdx, pIdx]() {

						chls[nextIdx][0]->asyncSend(&dummy[nextIdx], 1);
						std::cout << IoStream::lock;
						std::cout << myIdx << "| : " << "| thr[" << pIdx << "]:" << myIdx << " -> " << nextIdx << ": " << static_cast<int16_t>(dummy[nextIdx]) << "\n";
						std::cout << IoStream::unlock;
						send[nextIdx].init(opt, nParties, setSize, psiSecParam, bitSize, chls[nextIdx], otCountSend, otSend[nextIdx], otRecv[nextIdx], prng.get<block>(), false);
					});
				}
			}

			//last thread for connecting with leader
			u64 tLeaderIdx = pThrds.size() - 1;
			pThrds[pThrds.size() - 1] = std::thread([&, leaderIdx]() {

				chls[leaderIdx][0]->asyncSend(&dummy[leaderIdx], 1);

				std::cout << IoStream::lock;
				std::cout << myIdx << "| : " << "| thr[" << pThrds.size() - 1 << "]:" << myIdx << " --> " << leaderIdx << ": " << static_cast<int16_t>(dummy[leaderIdx]) << "\n";
				std::cout << IoStream::unlock;

				send[leaderIdx].init(opt, nParties, setSize, psiSecParam, bitSize, chls[leaderIdx], otCountSend, otSend[leaderIdx], otRecv[leaderIdx], prng.get<block>(), false);
			});

		}
		else
		{ //leader party 

			for (u64 pIdx = 0; pIdx < nSS; ++pIdx)
			{
				pThrds[pIdx] = std::thread([&, pIdx]() {
					chls[pIdx][0]->recv(&revDummy[pIdx], 1);
					std::cout << IoStream::lock;
					std::cout << myIdx << "| : " << "| thr[" << pIdx << "]:" << pIdx << " --> " << myIdx << ": " << static_cast<int16_t>(revDummy[pIdx]) << "\n";
					std::cout << IoStream::unlock;

					recv[pIdx].init(opt, nParties, setSize, psiSecParam, bitSize, chls[pIdx], otCountRecv, otRecv[pIdx], otSend[pIdx], ZeroBlock, false);
				});

			}
		}

		for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
			pThrds[pIdx].join();

		auto initDone = timer.setTimePoint("initDone");


#ifdef PRINT
		std::cout << IoStream::lock;
		if (myIdx == 0)
		{
			Log::out << myIdx << "| -> " << otSend[1].mGens[0].get<block>() << Log::endl;
			if (otRecv[1].hasBaseOts())
			{
				Log::out << myIdx << "| <- " << otRecv[1].mGens[0][0].get<block>() << Log::endl;
				Log::out << myIdx << "| <- " << otRecv[1].mGens[0][1].get<block>() << Log::endl;
			}
			Log::out << "------------" << Log::endl;
		}
		if (myIdx == 1)
		{
			if (otSend[0].hasBaseOts())
				Log::out << myIdx << "| -> " << otSend[0].mGens[0].get<block>() << Log::endl;

			Log::out << myIdx << "| <- " << otRecv[0].mGens[0][0].get<block>() << Log::endl;
			Log::out << myIdx << "| <- " << otRecv[0].mGens[0][1].get<block>() << Log::endl;
		}

		if (isDual)
		{
			if (myIdx == 0)
			{
				Log::out << myIdx << "| <->> " << otSend[tSS].mGens[0].get<block>() << Log::endl;
				if (otRecv[tSS].hasBaseOts())
				{
					Log::out << myIdx << "| <<-> " << otRecv[tSS].mGens[0][0].get<block>() << Log::endl;
					Log::out << myIdx << "| <<-> " << otRecv[tSS].mGens[0][1].get<block>() << Log::endl;
				}
				Log::out << "------------" << Log::endl;
			}
			if (myIdx == tSS)
			{
				if (otSend[0].hasBaseOts())
					Log::out << myIdx << "| <->> " << otSend[0].mGens[0].get<block>() << Log::endl;

				Log::out << myIdx << "| <<-> " << otRecv[0].mGens[0][0].get<block>() << Log::endl;
				Log::out << myIdx << "| <<-> " << otRecv[0].mGens[0][1].get<block>() << Log::endl;
			}
		}
		std::cout << IoStream::unlock;
#endif

#pragma endregion


		//##########################
		//### Hashing
		//##########################
		bins.hashing2Bins(set, 1);

		/*if(myIdx==0)
			bins.mSimpleBins.print(myIdx, true, false, false, false);
		if (myIdx == 1)
			bins.mCuckooBins.print(myIdx, true, false, false);*/

		auto hashingDone = timer.setTimePoint("hashingDone");

#pragma region compute OPRF

		//##########################
		//### Online Phasing - compute OPRF
		//##########################

		pThrds.clear();
		pThrds.resize(num_threads);
		if (myIdx == leaderIdx)
		{
			pThrds.resize(nParties - 1);
		}

		if (myIdx != leaderIdx)
		{
			for (u64 pIdx = 0; pIdx < tSS; ++pIdx)
			{
				u64 prevIdx = (myIdx - pIdx - 1 + nSS) % nSS;

				if (!(isDual && is_in_dual_area(idx_start_dual, idx_end_dual, nSS, prevIdx)))
				{
					u64 thr = t_prev_shift + pIdx;

					pThrds[thr] = std::thread([&, prevIdx]() {

						//prevIdx << " --> " << myIdx
						recv[prevIdx].getOPRFkeys(prevIdx, bins, chls[prevIdx], false);

					});
				}
			}

			for (u64 pIdx = 0; pIdx < tSS; ++pIdx)
			{
				u64 nextIdx = (myIdx + pIdx + 1) % nSS;

				if ((isDual && is_in_dual_area(idx_start_dual, idx_end_dual, nSS, nextIdx))) {

					pThrds[pIdx] = std::thread([&, nextIdx]() {
						//dual myIdx << " <-> " << nextIdx 
						if (myIdx < nextIdx)
						{
							send[nextIdx].getOPRFkeys(nextIdx, bins, chls[nextIdx], true);
						}
						else if (myIdx > nextIdx) //by index
						{
							recv[nextIdx].getOPRFkeys(nextIdx, bins, chls[nextIdx], true);
						}
					});

				}
				else
				{
					pThrds[pIdx] = std::thread([&, nextIdx]() {
						send[nextIdx].getOPRFkeys(nextIdx, bins, chls[nextIdx], false);

					});
				}
			}

			//last thread for connecting with leader
			pThrds[pThrds.size() - 1] = std::thread([&, leaderIdx]() {
				
				send[leaderIdx].getOPRFkeys(leaderIdx, bins, chls[leaderIdx], false);
			});

		}
		else
		{ //leader party 
			for (u64 pIdx = 0; pIdx < nSS; ++pIdx)
			{
				pThrds[pIdx] = std::thread([&, pIdx]() {
					recv[pIdx].getOPRFkeys(pIdx, bins, chls[pIdx], false);
				});
			}
		}

		for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
			pThrds[pIdx].join();

		auto getOPRFDone = timer.setTimePoint("getOPRFDone");


#ifdef BIN_PRINT

		if (myIdx == 0)
		{
			bins.mSimpleBins.print(1, true, true, false, false);
		}
		if (myIdx == 1)
		{
			bins.mCuckooBins.print(0, true, true, false);
		}

		if (isDual)
		{
			if (myIdx == 0)
			{
				bins.mCuckooBins.print(tSS, true, true, false);
			}
			if (myIdx == tSS)
			{
				bins.mSimpleBins.print(0, true, true, false, false);
			}
		}

#endif
#pragma endregion

#pragma region SS

		//##########################
		//### online phasing - secretsharing
		//##########################

		pThrds.clear();

		if (myIdx != leaderIdx)
		{
			pThrds.resize(num_threads);
			for (u64 pIdx = 0; pIdx < tSS; ++pIdx)
			{
				u64 prevIdx = (myIdx - pIdx - 1 + nSS) % nSS;

				if (!(isDual && is_in_dual_area(idx_start_dual, idx_end_dual, nSS, prevIdx)))
				{
					u64 thr = t_prev_shift + pIdx;

					pThrds[thr] = std::thread([&, prevIdx, pIdx]() {

						//prevIdx << " --> " << myIdx
						recv[prevIdx].recvSS(prevIdx, bins, recvPayLoads[pIdx], chls[prevIdx]);

					});
				}
			}

			for (u64 pIdx = 0; pIdx < tSS; ++pIdx)
			{
				u64 nextIdx = (myIdx + pIdx + 1) % nSS;

				if ((isDual && is_in_dual_area(idx_start_dual, idx_end_dual, nSS, nextIdx))) {

					pThrds[pIdx] = std::thread([&, nextIdx, pIdx]() {
						//dual myIdx << " <-> " << nextIdx 
						//send OPRF can receive payload
						if (myIdx < nextIdx)
						{
							send[nextIdx].sendSS(nextIdx, bins, sendPayLoads[pIdx], chls[nextIdx]);

							send[nextIdx].recvSS(nextIdx, bins, recvPayLoads[pIdx], chls[nextIdx]);
						}
						else if (myIdx > nextIdx) //by index
						{
							recv[nextIdx].recvSS(nextIdx, bins, recvPayLoads[pIdx], chls[nextIdx]);

							recv[nextIdx].sendSS(nextIdx, bins, sendPayLoads[pIdx], chls[nextIdx]);

						}
					});

				}
				else
				{
					pThrds[pIdx] = std::thread([&, nextIdx, pIdx]() {
						send[nextIdx].sendSS(nextIdx, bins, sendPayLoads[pIdx], chls[nextIdx]);
					});
				}
			}

			//last thread for connecting with leader
			pThrds[pThrds.size() - 1] = std::thread([&, leaderIdx]() {
				//send[leaderIdx].getOPRFKeys(leaderIdx, bins, chls[leaderIdx], false);
			});

			for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
				pThrds[pIdx].join();
		}

		auto getSSDone2Dir = timer.setTimePoint("secretsharingDone");


#ifdef PRINT
		std::cout << IoStream::lock;
		if (myIdx == 0)
		{
			for (int i = 0; i < 3; i++)
			{
				block temp = ZeroBlock;
				memcpy((u8*)&temp, (u8*)&sendPayLoads[0][i], maskSize);
				Log::out << myIdx << "| -> 1: (" << i << ", " << temp << ")" << Log::endl;
			}
			Log::out << "------------" << Log::endl;
		}
		if (myIdx == 1)
		{
			for (int i = 0; i < 3; i++)
			{
				block temp = ZeroBlock;
				memcpy((u8*)&temp, (u8*)&recvPayLoads[0][i], maskSize);
				Log::out << myIdx << "| <- 0: (" << i << ", " << temp << ")" << Log::endl;
			}
			Log::out << "------------" << Log::endl;
		}

		if (isDual)
		{
			/*if (myIdx == 0)
			{
				for (int i = 0; i < 3; i++)
				{
					block temp = ZeroBlock;
					memcpy((u8*)&temp, (u8*)&recvPayLoads[tSS][i], maskSize);
					Log::out << myIdx << "| <- "<< tSS<<": (" << i << ", " << temp << ")" << Log::endl;
				}
				Log::out << "------------" << Log::endl;
			}
			if (myIdx == tSS)
			{
				for (int i = 0; i < 3; i++)
				{
					block temp = ZeroBlock;
					memcpy((u8*)&temp, (u8*)&sendPayLoads[0][i], maskSize);
					Log::out << myIdx << "| -> 0: (" << i << ", " << temp << ")" << Log::endl;
				}
				Log::out << "------------" << Log::endl;
			}*/
		}

		std::cout << IoStream::unlock;
#endif
#pragma endregion

		//##########################
		//### online phasing - send XOR of zero share to leader
		//##########################
		pThrds.clear();

		if (myIdx != leaderIdx)
		{

			for (u64 i = 0; i < setSize; ++i)
			{
				//xor all received share
				for (u64 idxP = 0; idxP < tParties; ++idxP)
				{
					sendPayLoads[tParties][i] = sendPayLoads[tParties][i] ^ recvPayLoads[idxP][i];
				}
			}
			//send to leader
			send[leaderIdx].sendSS(leaderIdx, bins, sendPayLoads[tParties], chls[leaderIdx]);
		}
		else
		{
			pThrds.resize(nParties - 1);

			for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx) {
				pThrds[pIdx] = std::thread([&, pIdx]() {
					recv[pIdx].recvSS(pIdx, bins, recvPayLoads[pIdx], chls[pIdx]);
				});
			}

			for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
				pThrds[pIdx].join();
		}


		auto getSSDoneRound = timer.setTimePoint("leaderGetXorDone");


		//##########################
		//### online phasing - compute intersection
		//##########################

		if (myIdx == leaderIdx) {
			std::vector<u64> mIntersection;
			
			for (u64 i = 0; i < setSize; ++i)
			{

				//xor all received share
				block sum = ZeroBlock;
				for (u64 idxP = 0; idxP < nParties - 1; ++idxP)
				{
					sum = sum ^ recvPayLoads[idxP][i];
				}

				if (!memcmp((u8*)&ZeroBlock, &sum, bins.mMaskSize))
				{
					mIntersection.push_back(i);
				}
			}
			Log::out << "mIntersection.size(): " << mIntersection.size() << Log::endl;
		}
		auto getIntersection = timer.setTimePoint("getIntersection");


		

		//auto Mbps = dataSent * 8 / time / (1 << 20);


		if (myIdx == 0 || myIdx==leaderIdx) {
			auto offlineTime = std::chrono::duration_cast<std::chrono::milliseconds>(initDone - start).count();
			auto hashingTime = std::chrono::duration_cast<std::chrono::milliseconds>(hashingDone - initDone).count();
			auto getOPRFTime = std::chrono::duration_cast<std::chrono::milliseconds>(getOPRFDone - hashingDone).count();
			auto ss2DirTime = std::chrono::duration_cast<std::chrono::milliseconds>(getSSDone2Dir - getOPRFDone).count();
			auto ssRoundTime = std::chrono::duration_cast<std::chrono::milliseconds>(getSSDoneRound - getSSDone2Dir).count();
			auto intersectionTime = std::chrono::duration_cast<std::chrono::milliseconds>(getIntersection - getSSDoneRound).count();

			double onlineTime = hashingTime + getOPRFTime + ss2DirTime + ssRoundTime + intersectionTime;

			double time = offlineTime + onlineTime;
			time /= 1000;

			u64 dataSent = 0;

			for (u64 i = 0; i < nParties; ++i)
			{
				if (i != myIdx) {
					chls[i].resize(numThreads);
					for (u64 j = 0; j < numThreads; ++j)
					{
						dataSent += chls[i][j]->getTotalDataSent();
					}
				}
			}
			auto Mbps = dataSent * 8 / time / (1 << 20);

			std::cout << setSize << "  " << offlineTime << "  " << onlineTime << "        " << Mbps << " Mbps      " << (dataSent / std::pow(2.0, 20)) << " MB" << std::endl;

			for (u64 i = 0; i < nParties; ++i)
			{
				if (i != myIdx) {
					chls[i].resize(numThreads);
					for (u64 j = 0; j < numThreads; ++j)
					{
						chls[i][j]->resetStats();
					}
				}
			}

			std::cout << "setSize: " << setSize << "\n"
				<< "offlineTime:  " << offlineTime << " ms\n"
				<< "hashingTime:  " << hashingTime << " ms\n"
				<< "getOPRFTime:  " << getOPRFTime << " ms\n"
				<< "ss2DirTime:  " << ss2DirTime << " ms\n"
				<< "ssRoundTime:  " << ssRoundTime << " ms\n"
				<< "intersection:  " << intersectionTime << " ms\n"
				<< "onlineTime:  " << onlineTime << " ms\n"
				<< "Total time: " << time << " s\n"
				<< "------------------\n";


			offlineAvgTime += offlineTime;
			hashingAvgTime += hashingTime;
			getOPRFAvgTime += getOPRFTime;
			ss2DirAvgTime += ss2DirTime;
			ssRoundAvgTime += ssRoundTime;
			intersectionAvgTime += intersectionTime;
			onlineAvgTime += onlineTime;

		}

		}


	/*if (myIdx == 0) {
	double avgTime = (offlineAvgTime + onlineAvgTime);
	avgTime /= 1000;
	std::cout << "=========avg==========\n"
	<< "setSize: " << setSize << "\n"
	<< "offlineTime:  " << offlineAvgTime / numTrial << " ms\n"
	<< "hashingTime:  " << hashingAvgTime / numTrial << " ms\n"
	<< "getOPRFTime:  " << getOPRFAvgTime / numTrial << " ms\n"
	<< "ss2DirTime:  " << ss2DirAvgTime << " ms\n"
	<< "ssRoundTime:  " << ssRoundAvgTime << " ms\n"
	<< "intersection:  " << intersectionAvgTime / numTrial << " ms\n"
	<< "onlineTime:  " << onlineAvgTime / numTrial << " ms\n"
	<< "Total time: " << avgTime / numTrial << " s\n";
	runtime << "setSize: " << setSize << "\n"
	<< "offlineTime:  " << offlineAvgTime / numTrial << " ms\n"
	<< "hashingTime:  " << hashingAvgTime / numTrial << " ms\n"
	<< "getOPRFTime:  " << getOPRFAvgTime / numTrial << " ms\n"
	<< "ss2DirTime:  " << ss2DirAvgTime << " ms\n"
	<< "ssRoundTime:  " << ssRoundAvgTime << " ms\n"
	<< "intersection:  " << intersectionAvgTime / numTrial << " ms\n"
	<< "onlineTime:  " << onlineAvgTime / numTrial << " ms\n"
	<< "Total time: " << avgTime / numTrial << " s\n";
	runtime.close();
	}
	*/
	for (u64 i = 0; i < nParties; ++i)
	{
		if (i != myIdx)
		{
			for (u64 j = 0; j < numThreads; ++j)
			{
				chls[i][j]->close();
			}
		}
	}

	for (u64 i = 0; i < nParties; ++i)
	{
		if (i != myIdx)
			ep[i].stop();
	}


	ios.stop();
	}
void OPPRFnt_EmptrySet_Test_Impl()
{
	u64 setSize = 1 << 5, psiSecParam = 40, bitSize = 128;
	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
	mSet.resize(setSize);
	for (u64 i = 0; i < setSize; ++i)
	{
		mSet[i] = prng.get<block>();
	}
	nParties = 5;
	u64 tParties = 1;

	if (tParties == nParties - 1)//max ss = n-1
		tParties--;
	else if (tParties < 1) //make sure to do ss with at least one client
		tParties = 1;
	std::vector<std::thread>  pThrds(nParties);
	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
	{
		//if (pIdx == 0)
		//{
		//	//tparty0(pIdx, nParties, 1, setSize, mSet);
		//}
		//else
		{
			pThrds[pIdx] = std::thread([&, pIdx]() {
				//	Channel_party_test(pIdx);
				tparty(pIdx, nParties, tParties, mSet.size(), mSet, 2);
			});
		}
	}
	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
		pThrds[pIdx].join();


}

void OPPRFn_Aug_EmptrySet_Test_Impl()
{
	u64 setSize = 1 <<5, psiSecParam = 40, bitSize = 128;
	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
	mSet.resize(setSize);
	for (u64 i = 0; i < setSize; ++i)
	{
		mSet[i] = prng.get<block>();
	}
	
	nParties = 3;

	std::vector<std::vector<block>> mSeeds(nParties);
	std::vector<std::vector<PRNG>> mPRNGSeeds(nParties);
	for (u64 i = 0; i < nParties; ++i)
	{
		mSeeds[i].resize(nParties);
		for (u64 j = 0; j < nParties; ++j)
		{
			if (i <= j)
				mSeeds[i][j] = prng.get<block>();
			else
				mSeeds[i][j] = mSeeds[j][i];
		}
	}
	for (u64 i = 0; i < nParties; ++i)
	{		
		mPRNGSeeds[i].resize(nParties);
		for (u64 j = 0; j < nParties; ++j)
		{
			mPRNGSeeds[i][j].SetSeed(mSeeds[i][j]);
		}
	}

	for (u64 i = 0; i < 1; ++i)
	{
		std::vector<block> sum(nParties);
		for (u64 mIdx = 0;mIdx < nParties; mIdx++)
		{
			sum[mIdx] = ZeroBlock;
			for (u64 pIdx = 0; pIdx < nParties; pIdx++)
			{
				if (pIdx != mIdx)
				{
					//sum = sum ^ mSeedPrng[pIdx].get<block>();
					sum[mIdx] = sum[mIdx]^ mPRNGSeeds[mIdx][pIdx].get<block>();
				}
			}
		}
		block final_sum = ZeroBlock;
		for (u64 mIdx = 0; mIdx < nParties; mIdx++)
		{
			final_sum = final_sum^sum[mIdx];
		}
		std::cout << final_sum << std::endl;

		
	}


	std::vector<std::thread>  pThrds(nParties);
	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
	{
		pThrds[pIdx] = std::thread([&, pIdx]() {
				//	Channel_party_test(pIdx);
			aug_party(pIdx, nParties, mSet.size(), mSet, mPRNGSeeds[pIdx]);
			});
	}
	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
		pThrds[pIdx].join();


}


void Channel_party_test(u64 myIdx)
{
	u64 setSize = 1 << 5, psiSecParam = 40, bitSize = 128, numThreads = 1;
	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));


	std::vector<u8> dummy(nParties);
	std::vector<u8> revDummy(nParties);


	std::string name("psi");
	BtIOService ios(0);

	int btCount = nParties;
	std::vector<BtEndpoint> ep(nParties);

	for (u64 i = 0; i < nParties; ++i)
	{
		dummy[i] = myIdx * 10 + i;
		if (i < myIdx)
		{
			u32 port = i * 10 + myIdx;//get the same port; i=1 & pIdx=2 =>port=102
			ep[i].start(ios, "localhost", port, false, name); //channel bwt i and pIdx, where i is sender
		}
		else if (i > myIdx)
		{
			u32 port = myIdx * 10 + i;//get the same port; i=2 & pIdx=1 =>port=102
			ep[i].start(ios, "localhost", port, true, name); //channel bwt i and pIdx, where i is receiver
		}
	}


	std::vector<std::vector<Channel*>> chls(nParties);

	for (u64 i = 0; i < nParties; ++i)
	{
		if (i != myIdx) {
			chls[i].resize(numThreads);
			for (u64 j = 0; j < numThreads; ++j)
			{
				//chls[i][j] = &ep[i].addChannel("chl" + std::to_string(j), "chl" + std::to_string(j));
				chls[i][j] = &ep[i].addChannel(name, name);
			}
		}
	}



	std::mutex printMtx1, printMtx2;
	std::vector<std::thread>  pThrds(nParties);
	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
	{
		pThrds[pIdx] = std::thread([&, pIdx]() {
			if (pIdx < myIdx) {


				chls[pIdx][0]->asyncSend(&dummy[pIdx], 1);
				//std::lock_guard<std::mutex> lock(printMtx1);
			//	std::cout << "s: " << myIdx << " -> " << pIdx << " : " << static_cast<int16_t>(dummy[pIdx]) << std::endl;

			}
			else if (pIdx > myIdx) {

				chls[pIdx][0]->recv(&revDummy[pIdx], 1);
				std::lock_guard<std::mutex> lock(printMtx2);
				std::cout << "r: " << myIdx << " <- " << pIdx << " : " << static_cast<int16_t>(revDummy[pIdx]) << std::endl;

			}
		});
	}


	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
	{
		//	if(pIdx!=myIdx)
		pThrds[pIdx].join();
	}




	for (u64 i = 0; i < nParties; ++i)
	{
		if (i != myIdx)
		{
			for (u64 j = 0; j < numThreads; ++j)
			{
				chls[i][j]->close();
			}
		}
	}

	for (u64 i = 0; i < nParties; ++i)
	{
		if (i != myIdx)
			ep[i].stop();
	}


	ios.stop();
}

void OPPRFn_EmptrySet_Test_Impl()
{
	u64 setSize = 1 << 5, psiSecParam = 40, bitSize = 128;
	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
	mSet.resize(setSize);
	for (u64 i = 0; i < setSize; ++i)
	{
		mSet[i] = prng.get<block>();
	}
	std::vector<std::thread>  pThrds(nParties);
	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
	{
		pThrds[pIdx] = std::thread([&, pIdx]() {
			//	Channel_party_test(pIdx);
			party(pIdx, setSize, mSet);
		});
	}
	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
		pThrds[pIdx].join();


}

void OPPRF3_EmptrySet_Test_Impl()
{
	nParties = 3;
	u64 setSize = 1 << 5, psiSecParam = 40, bitSize = 128;

	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
	mSet.resize(setSize);
	for (u64 i = 0; i < setSize; ++i)
	{
		mSet[i] = prng.get<block>();
	}
	std::vector<std::thread>  pThrds(nParties);
	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
	{
		pThrds[pIdx] = std::thread([&, pIdx]() {
			//	Channel_party_test(pIdx);
			party3(pIdx, setSize, mSet);
		});
	}
	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
		pThrds[pIdx].join();


}


void OPPRF2_EmptrySet_Test_Impl()
{
	nParties = 2;
	u64 setSize = 1 << 5, psiSecParam = 40, bitSize = 128;

	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
	mSet.resize(setSize);
	for (u64 i = 0; i < setSize; ++i)
	{
		mSet[i] = prng.get<block>();
	}
	std::vector<std::thread>  pThrds(nParties);
	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
	{
		pThrds[pIdx] = std::thread([&, pIdx]() {
			//	Channel_party_test(pIdx);
			party2(pIdx, setSize, mSet);
		});
	}
	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
		pThrds[pIdx].join();


}

void GF2EFromBlock(NTL::GF2E &element, block& blk,u64 size) {

	NTL::GF2X x1;
	NTL::BuildIrred(x1, 40);
	NTL::GF2E::init(x1);
	//convert the Block to GF2X element.
	NTL::GF2XFromBytes(x1, (u8*)&blk, size);
	element = to_GF2E(x1);
}



void BlockFromGF2E(block& blk, NTL::GF2E & element, u64 size) {
	//Get the bytes of the random element.
	


	//int numBytes = NTL::NumBytes(element); //get the number of element bytes.

	//vector<uint8_t> arr(numBytes);
	////the function rep returns the representation of GF2E as the related GF2X, it returns as read only.
	//BytesFromGF2X(arr.data(), element, numBytes);


	NTL::GF2X fromEl = NTL::rep(element); //convert the GF2E element to GF2X element.	
										  //the function rep returns the representation of GF2E as the related GF2X, it returns as read only.
	
	int numBytes = NTL::NumBytes(fromEl);
	
	BytesFromGF2X((u8*)&blk, fromEl, size);
}

u64 masksize = 5;

//computes coefficients (in blocks) of f such that f(x[i]) = y[i]
void getBlkCoefficients( std::vector<block>& setX, std::vector<block>& setY, std::vector<block>& coeffs)
{
	NTL::vec_GF2E x; NTL::vec_GF2E y;
	NTL::GF2E e;

	for (u64 i = 0; i < setX.size(); ++i)
	{
		GF2EFromBlock(e, setX[i], masksize);
		x.append(e);

		GF2EFromBlock(e, setY[i], masksize);
		y.append(e);
	}

	
	//interpolate
	NTL::GF2EX polynomial = NTL::interpolate(x, y);


	////convert coefficient to vector<block> 
	coeffs.resize(NTL::deg(polynomial) + 1);
	for (int i = 0; i < coeffs.size(); i++) {
		//get the coefficient polynomial
		e = NTL::coeff(polynomial, i);
		NTL::GF2X fromEl = NTL::rep(e);
		int numBytes = NTL::NumBytes(fromEl);
		std::cout << "numBytes" << numBytes << std::endl;
		BlockFromGF2E(coeffs[i], e, masksize);
	}
}

//compute y=f(x) giving coefficients (in block)
void evalPolynomial(std::vector<block>& coeffs, block& x, block& y)
{
	NTL::GF2EX res_polynomial;
	NTL::GF2E e;
	for (u64 i = 0; i < coeffs.size(); ++i)
	{
		GF2EFromBlock(e, coeffs[i], masksize);
		NTL::SetCoeff(res_polynomial, i, e); //build res_polynomial
	}

	GF2EFromBlock(e, x, masksize);
	e = NTL::eval(res_polynomial, e); //get y=f(x) in GF2E
	BlockFromGF2E(y, e, masksize); //convert to block 
}

void polynomial_Test_Impl()
{	
	
	std::vector<u32> test;
	test.emplace_back(0);
	test.emplace_back(0);
	test.emplace_back(1);
	std::cout << test.size() << std::endl;
	u64 setSize = 1 << 12;
	std::cout <<  std::log2(setSize);


#if 0
	std::vector<block> mSetX, mSetY;

	u64 setSize = 10;
	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
	mSetX.resize(setSize);
	mSetY.resize(setSize);
	for (u64 i = 0; i < setSize; ++i)
	{
		mSetX[i] = prng.get<block>();
		mSetY[i] = prng.get<block>();
		//	std::cout << mSetY[i] << std::endl;
	}

	NTL::GF2E e1;
	block blkk;
	GF2EFromBlock(e1, mSetX[0], masksize);
	BlockFromGF2E(blkk, e1, masksize);
	std::cout << mSetX[0] <<"---"<< blkk << std::endl;


	std::cout << std::endl;
	std::vector<block> coeffs;	
	
	getBlkCoefficients( mSetX, mSetY, coeffs);

	block blkY;
	for (int i = 0; i < mSetY.size(); ++i)
	{

		evalPolynomial(coeffs, mSetX[i], blkY);

		std::cout << blkY << " -- " << mSetY[i] << std::endl;

		//if (neq(blkY, mSetY[i]))
		//	throw UnitTestFail();
	}
	//block blkX = prng.get<block>();
	//evalPolynomial(coeffs, blkX, blkY);
	//std::cout << blkX << " -- " << blkY << std::endl;

	/*if (eq(blkX, blkY))
	throw UnitTestFail();*/

	//NTL::GF2EX p1;
	//NTL::random(p1, 1);
	//std::cout << NTL::deg(p1) << std::endl;


	//for (u32 i = 0; i < NTL::deg(p1) + 1; i++)
	//{
	//	NTL::GF2E x1;
	//	NTL::GetCoeff(x1, p1, 0);
	//	std::cout << x1 << std::endl;
	//}


	//NTL::GF2EX p2;
	//NTL::random(p2, 1);
	//std::cout << NTL::deg(p2) << std::endl;

	//for (u32 i = 0; i < NTL::deg(p2) + 1; i++)
	//{
	//	NTL::GF2E x1;
	//	NTL::GetCoeff(x1, p2, 0);
	//	std::cout << x1 << std::endl;
	//}

	//NTL::GF2EX p3;
	//NTL::mul(p3, p1, p2);
	//std::cout << NTL::deg(p3) << std::endl;

	//for (u32 i = 0; i < NTL::deg(p3) + 1; i++)
	//{
	//	NTL::GF2E x1;
	//	NTL::GetCoeff(x1, p3, 0);
	//	std::cout << x1 << std::endl;
	//}

	//block test = prng.get<block>();
	//NTL::GF2X xi;
	//NTL::GF2XFromBytes(xi, (u8*)&test, sizeof(block));
#endif // 0
}

//void PlainMul(GF2EX&, const GF2EX&, const GF2EX&);
//
//void GF2X_Test_Impl()
//{
//	NTL::GF2X p;
//
//	NTL::BuildIrred(p, 200);
//
//	NTL::GF2E::init(p);
//
//	NTL::GF2EX f;
//
//	NTL::SetCoeff(f, 41);
//	NTL::SetCoeff(f, 1);
//	NTL::SetCoeff(f, 0);
//
//	NTL::GF2X a;
//	NTL::SetCoeff(a, 117);
//	NTL::SetCoeff(a, 10);
//	NTL::SetCoeff(a, 0);
//
//	NTL::GF2EX g, h;
//	NTL::SetX(g);
//	NTL::SetCoeff(g, 0, to_GF2E(a));
//
//	MinPolyMod(h, g, f);
//
//	f = h;
//
//	vec_pair_GF2EX_long u;
//
//	CanZass(u, f, 1);
//
//	cerr << "factorization pattern:";
//	long i;
//
//	for (i = 0; i < u.length(); i++) {
//		cerr << " ";
//		long k = u[i].b;
//		if (k > 1)
//			cerr << k << "*";
//		cerr << deg(u[i].a);
//	}
//
//	cerr << "\n\n\n";
//
//	GF2EX ff;
//	mul(ff, u);
//
//	if (f != ff || u.length() != 11) {
//		cerr << "GF2EXTest NOT OK\n";
//		return 1;
//	}
//
//	{
//
//		cerr << "multiplication test...\n";
//
//		NTL::BuildIrred(p, 512);
//		NTL::GF2E::init(p);
//
//		NTL::GF2EX A, B, C, C1;
//
//
//		NTL::random(A, 512);
//		NTL::random(B, 512);
//
//		double t;
//		long i;
//
//		
//		for (i = 0; i < 10; i++) PlainMul(C, A, B);
//		t = GetTime() - t;
//		cerr << "time for plain mul of degree 511 over GF(2^512): " << (t / 10) << "s\n";
//
//		t = GetTime();
//		for (i = 0; i < 10; i++) mul(C1, A, B);
//		t = GetTime() - t;
//		cerr << "time for karatsuba mul of degree 511 over GF(2^512): " << (t / 10) << "s\n";
//
//		if (C != C1) {
//			cerr << "GF2EXTest NOT OK\n";
//			return 1;
//		}
//
//	}
//
//	{
//
//		cerr << "multiplication test...\n";
//
//		BuildIrred(p, 16);
//		GF2E::init(p);
//
//		GF2EX A, B, C, C1;
//
//
//		random(A, 512);
//		random(B, 512);
//
//		double t;
//
//		t = GetTime();
//		for (i = 0; i < 10; i++) PlainMul(C, A, B);
//		t = GetTime() - t;
//		cerr << "time for plain mul of degree 511 over GF(2^16): " << (t / 10) << "s\n";
//
//		t = GetTime();
//		for (i = 0; i < 10; i++) mul(C1, A, B);
//		t = GetTime() - t;
//		cerr << "time for karatsuba mul of degree 511 over GF(2^16): " << (t / 10) << "s\n";
//
//		if (C != C1) {
//			std::cout << "GF2EXTest NOT OK\n";
//			return 1;
//		}
//
//	}
//
//	std::cout << "GF2EXTest OK\n";
//}

void Unconditional_0_Sharing_Test_Impl()
{
	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
	
	//std::vector<std::vector<block>> prngSeeds;

	block x=prng.get<block>();
	//block z = prng.get<block>();
	block y = ZeroBlock^x;// ^z;

	block test = x^y;// ^z;
	std::cout << test<< std::endl;

	PRNG prng1, prng2, prng3;
	prng1.SetSeed(x);
	prng2.SetSeed(y);
	//prng3.SetSeed(z);

	for (u32 i = 0; i < 10; i++)
	{
		block test1 = prng1.get<block>();
		block test2 = prng2.get<block>();
		//	block test3= prng3.get<block>();
		test = test1^test2;// ^test3;
		std::cout << test << std::endl;
	}

}

void getGBF(u64 bfBitCount, std::vector<block>& setX, std::vector<block>& setY, std::vector<block>& garbledBF) {

}
void GBF_Test_Impl()
{

	u64 numHashFunctions = 4;
	u64 setSize = 10;
	u64 mBfBitCount = numHashFunctions * setSize;
	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));

	std::vector<block> mSetX(setSize),mSetY(setSize), garbledBF(mBfBitCount); //h(x) and value y
	
	for (u64 i = 0; i < setSize; ++i)
	{
		mSetX[i] = prng.get<block>();
		mSetY[i] = prng.get<block>();
	}

	//create hash
	
	std::vector<AES> mBFHasher(numHashFunctions);
	for (u64 i = 0; i < mBFHasher.size(); ++i)
		mBFHasher[i].setKey(_mm_set1_epi64x(i));

		

	std::vector<std::set<u64>> idxs(setSize);
	for (u64 i = 0; i < 1; ++i)
	{
		u64 firstFreeIdx(-1);
		block sum = ZeroBlock;

		//std::cout << "input[" << i << "] " << inputs[i] << std::endl;

		//idxs.clear();
		for (u64 hashIdx = 0; hashIdx < mBFHasher.size(); ++hashIdx)
		{	

			block hashOut = mBFHasher[hashIdx].ecbEncBlock(mSetX[i]);
			u64& idx = *(u64*)&hashOut;
			idx %= mBfBitCount;
			idxs[i].emplace(idx);

			std::cout << idx << " ";
		}
		std::cout << "\n";

		block test=ZeroBlock;
		for (auto idx : idxs[i])
		{
			if (eq(garbledBF[idx], ZeroBlock))
			{
				if (firstFreeIdx == u64(-1))
				{
					firstFreeIdx = idx;
					std::cout << "firstFreeIdx: " << firstFreeIdx << std::endl;

				}
				else
				{
					garbledBF[idx] = _mm_set_epi64x(idx,idx);
				//	std::cout << garbledBF[idx] <<"\n";
					test = test^garbledBF[idx];
					sum = sum ^ garbledBF[idx];
					std::cout << idx << " " << garbledBF[idx] << std::endl;
				}
			}			
			else
			{
				sum = sum ^ garbledBF[idx];
				test = test^garbledBF[idx];
				std::cout << idx << " " << garbledBF[idx] << std::endl;
			}		
		}

		garbledBF[firstFreeIdx] = sum^mSetY[i];
		std::cout << firstFreeIdx << " " << garbledBF[firstFreeIdx] << std::endl;
		test = test^garbledBF[firstFreeIdx];
		std::cout << test << "\n";
		//std::cout << "sender " << i << " *   " << garbledBF[firstFreeIdx] << "    " << firstFreeIdx << std::endl;
	}

	//test
	for (u64 i = 0; i < 1; ++i)
	{
		std::cout <<"mSetY["<<i<<"]= "<< mSetY[i] << std::endl;
	//	std::cout << mSetX[i] << std::endl;

		block sum=ZeroBlock;
		for (auto idx : idxs[i])
		{
			std::cout << idx << " " << garbledBF[idx] << std::endl;
			sum = sum^garbledBF[idx];
		}
		std::cout << sum << std::endl;
	}
	/*for (u64 i = 0; i < garbledBF.size(); ++i)
	{
		if (eq(garbledBF[i], ZeroBlock))
			garbledBF[i] = prng.get<block>();
	}*/

}
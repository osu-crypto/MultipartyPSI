#include "nPSI.h"

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


using namespace osuCrypto;
#define PRINT

u32 opt = 0;
void nOPPRF2_EmptrySet_Test_Impl()
{
    u64 setSize = 1<<5, psiSecParam = 40, bitSize = 128 , numParties=2;
    PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));

    std::vector<block> sendSet(setSize), recvSet(setSize);
	std::vector<block> sendPayLoads(setSize), recvPayLoads(setSize);

    for (u64 i = 0; i < setSize; ++i)
    {
        sendSet[i] = prng.get<block>();
		sendPayLoads[i]= prng.get<block>();
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
 //   std::thread thrd([&]() {


	//	send.init(opt, numParties,setSize, psiSecParam, bitSize, sendChl, otSend0, otRecv1, prng.get<block>());
	//	send.hash2Bins(sendSet, sendChl);
		//send.getOPRFKeys(1,sendChl);
		//send.sendSSTableBased(1, sendPayLoads, sendChl);
	//	send.recvSSTableBased(1, recvPayLoads, sendChl);
		//Log::out << "send.mSimpleBins.print(true, false, false,false);" << Log::endl;
	//	send.mSimpleBins.print(1,true, true, true, true);
		//Log::out << "send.mCuckooBins.print(true, false, false);" << Log::endl;
		//send.mCuckooBins.print(1,true, true, false);
 //   });
//	recv.init(opt, numParties,setSize, psiSecParam, bitSize, recvChl, otRecv0, otSend1, ZeroBlock);
	//recv.hash2Bins(recvSet, recvChl);
//	recv.getOPRFkeys(0, recvChl);
	//recv.recvSSTableBased(0, recvPayLoads, recvChl);
//	recv.sendSSTableBased(0, sendPayLoads, recvChl);

	Log::out << "recv.mCuckooBins.print(true, false, false);" << Log::endl;
//	recv.mCuckooBins.print(0,true, true, false);
	
	//Log::out << "recv.mSimpleBins.print(true, false, false,false);" << Log::endl;
	//recv.mSimpleBins.print(0,true, true, true, true);



	//std::vector<std::thread>  pThrds(numParties);

	//for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
	//{
	//	pThrds[pIdx] = std::thread([&, pIdx]() {
	//		if (pIdx == 0)
	//		{
	//			send.init(opt, numParties, setSize, psiSecParam, bitSize, sendChl, otSend0, otRecv1, prng.get<block>());

	//		}
	//		else if (pIdx == 1) {
	//			recv.init(opt, numParties, setSize, psiSecParam, bitSize, recvChl, otRecv0, otSend1, ZeroBlock);
	//		}
	//	});
	//}

	//for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
	//	pThrds[pIdx].join();


	 


#ifdef PRINT
std::cout << IoStream::lock;
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
	Log::out << otSend0.mT.size()[1] << Log::endl;
	Log::out << otSend1.mT.size()[1] << Log::endl;
	Log::out << otSend0.mGens[0].get<block>() << Log::endl;
	Log::out << otRecv0.mGens[0][0].get<block>() << Log::endl;
	Log::out << otRecv0.mGens[0][1].get<block>() << Log::endl;
	Log::out << "------------" << Log::endl;
	Log::out << otSend1.mGens[0].get<block>() << Log::endl;
	Log::out << otRecv1.mGens[0][0].get<block>() << Log::endl;
	Log::out << otRecv1.mGens[0][1].get<block>() << Log::endl;
	std::cout << IoStream::unlock;

#endif

//	thrd.join();

	

   



    sendChl[0]->close();
    recvChl[0]->close();

    ep0.stop();
    ep1.stop();
    ios.stop();
}
void nOPPRF_EmptrySet_Test_Impl()
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
	BtIOService ios1(0);
	BtIOService ios2(0);
	BtEndpoint sendEP[3];
	BtEndpoint recvEP[3];

	sendEP[0].start(ios, "localhost", 01, true, name);
	sendEP[1].start(ios1, "localhost", 02, true, name);
	sendEP[2].start(ios2, "localhost", 12, true, name);

	recvEP[0].start(ios, "localhost", 01, false, name);
	recvEP[1].start(ios1, "localhost", 02, false, name);
	recvEP[2].start(ios2, "localhost", 12, false, name);




	std::vector<Channel*> recvChl[3];
	recvChl[0] = { &recvEP[0].addChannel(name, name) };
	recvChl[1] = { &recvEP[1].addChannel(name, name) };
	recvChl[2]=	{ &recvEP[2].addChannel(name, name) };
	std::vector<Channel*> sendChl[3];
	sendChl[0]={ &sendEP[0].addChannel(name, name) };
	sendChl[1] = { &sendEP[1].addChannel(name, name) };
	sendChl[2] = { &sendEP[2].addChannel(name, name) };


	KkrtNcoOtReceiver otRecv0[3], otRecv1[3];
	KkrtNcoOtSender otSend0[3], otSend1[3];
	OPPRFSender send[3][3];
	OPPRFReceiver recv[3][3];	

	//KkrtNcoOtReceiver otRecv02, otRecv12;
	//KkrtNcoOtSender otSend02, otSend12;
	//OPPRFSender send2;
	//OPPRFReceiver recv2;

	Log::out << "sendPayLoads[i]" << Log::endl;
	std::vector<std::thread>  pThrds(3);

	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
	{
		pThrds[pIdx] = std::thread([&, pIdx]() {
			if (pIdx == 0)
			{
				std::vector<std::thread>  pThrds0(2);
				for (u64 pIdx0 = 0; pIdx0 < pThrds0.size(); ++pIdx0)
				{
					pThrds0[pIdx0] = std::thread([&, pIdx0]() {
						send[0][pIdx0+1].init(opt,numParties, setSize, psiSecParam, bitSize, sendChl[pIdx0], 1.5*setSize, otSend0[pIdx0], otRecv1[pIdx0], prng.get<block>());
						//send[0][2].init(opt, numParties, setSize, psiSecParam, bitSize, sendChl2, otSend02, otRecv12, prng.get<block>());
					});
				}
				for (u64 pIdx0 = 0; pIdx0 < pThrds0.size(); ++pIdx0)
					pThrds0[pIdx0].join();


			//	send[0][1].init(opt, numParties, setSize, psiSecParam, bitSize, sendChl[0], otSend0[0], otRecv1[0], prng.get<block>());
			//	send[0][2].init(opt, numParties, setSize, psiSecParam, bitSize, sendChl[1], otSend0[1], otRecv1[1], prng.get<block>());


			}
			else if (pIdx == 1) {
				std::vector<std::thread>  pThrds1(3);
				for (u64 pIdx0 = 0; pIdx0 < pThrds1.size(); ++pIdx0)
				{
					pThrds1[pIdx0] = std::thread([&, pIdx0]() {
						if (pIdx0 == 0)
							recv[pIdx][0].init(opt, numParties, setSize, psiSecParam, bitSize, recvChl[0], 1.5*setSize, otRecv0[0], otSend1[0], ZeroBlock);
						else if (pIdx0 == 2)
							send[pIdx][2].init(opt, numParties, setSize, psiSecParam, bitSize, sendChl[2], 1.5*setSize, otSend0[2], otRecv1[2], prng.get<block>());
					});
				}
					for (u64 pIdx0 = 0; pIdx0 < pThrds1.size(); ++pIdx0)
						pThrds1[pIdx0].join();
			}
			else if (pIdx == 2) {
				recv[pIdx][0].init(opt, numParties, setSize, psiSecParam, bitSize, recvChl[1], 1.5*setSize, otRecv0[1], otSend1[1], ZeroBlock);
				recv[pIdx][1].init(opt, numParties, setSize, psiSecParam, bitSize, recvChl[2], 1.5*setSize, otRecv0[2], otSend1[2], ZeroBlock);

			}
		});
	}

	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
		pThrds[pIdx].join();


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
	Log::out << otSend0[0].mT.size()[1] << Log::endl;
	Log::out << otSend1[0].mT.size()[1] << Log::endl;
	Log::out << otSend0[0].mGens[0].get<block>() << Log::endl;
	Log::out << otRecv0[0].mGens[0][0].get<block>() << Log::endl;
	Log::out << otRecv0[0].mGens[0][1].get<block>() << Log::endl;
	Log::out << "------------" << Log::endl;
	Log::out << otSend1[0].mGens[0].get<block>() << Log::endl;
	Log::out << otRecv1[0].mGens[0][0].get<block>() << Log::endl;
	Log::out << otRecv1[0].mGens[0][1].get<block>() << Log::endl;
	Log::out << "------------" << Log::endl;
	Log::out << otSend0[1].mGens[0].get<block>() << Log::endl;
	Log::out << otRecv0[1].mGens[0][0].get<block>() << Log::endl;
	Log::out << otRecv0[1].mGens[0][1].get<block>() << Log::endl;
	Log::out << "------------" << Log::endl;
	Log::out << otSend1[1].mGens[0].get<block>() << Log::endl;
	Log::out << otRecv1[1].mGens[0][0].get<block>() << Log::endl;
	Log::out << otRecv1[1].mGens[0][1].get<block>() << Log::endl;
	//Log::out << "------------" << Log::endl;
	//Log::out << otSend1[2].mGens[0].get<block>() << Log::endl;
	//Log::out << otRecv1[2].mGens[0][0].get<block>() << Log::endl;
	//Log::out << otRecv1[2].mGens[0][1].get<block>() << Log::endl;

	std::cout << IoStream::unlock;

#endif

	//	thrd.join();



	sendChl[0][0]->close();
	recvChl[0][0]->close();
	sendChl[1][0]->close();
	recvChl[1][0]->close();
	sendChl[2][0]->close();
	recvChl[2][0]->close();

	sendEP[1].stop();
	sendEP[0].stop();
	sendEP[2].stop();
	recvEP[0].stop();
	recvEP[1].stop();
	recvEP[2].stop();
	ios.stop(); ios1.stop(); ios2.stop();
}

void nOPPRF3_EmptrySet_Test_Impl()
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

void nOPPRF_EmptrySet_hashing_Test_Impl()
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
//
//		bins[0].init(0, numParties, setSize, psiSecParam,opt);
//		u64 otCounts = bins[0].mSimpleBins.mBins.size();
//		send[0].init(opt, numParties, setSize, psiSecParam, bitSize, sendChl, otCounts,otSend[0], otRecv[0], prng.get<block>());
//
//
//		bins[0].hashing2Bins(sendSet, 2);
//		//send.hash2Bins(sendSet, sendChl);
//		send[0].getOPRFKeys(1, bins[0], sendChl,true);
//		send[0].sendSSTableBased(1, bins[0], sendPayLoads, sendChl);
//		//send.recvSSTableBased(1, recvPayLoads, sendChl);
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
//	bins[1].init(0, numParties, setSize, psiSecParam,opt);
//
//	u64 otCountRecv = bins[1].mCuckooBins.mBins.size();
//
//	recv[0].init(opt, numParties, setSize, psiSecParam, bitSize, recvChl, otCountRecv,otRecv[1], otSend[1], ZeroBlock);
//
//	bins[1].hashing2Bins(recvSet, 2);
//
//	//recv.hash2Bins(recvSet, recvChl);
//	recv[0].getOPRFkeys(0, bins[1], recvChl);
//	recv[0].recvSSTableBased(0, bins[1], recvPayLoads, recvChl);
//	//recv.sendSSTableBased(0, sendPayLoads, recvChl);
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

std::vector<block> mSet2;
u64 mParties(2);

void nParty(u64 myIdx)
{
	u64 setSize = 1 << 5, psiSecParam = 40, bitSize = 128, numThreads = 1;
	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));

	std::vector<block> set(setSize);
	std::vector<u8> dummy(mParties);
	std::vector<u8> revDummy(mParties);
	std::vector<std::vector<block>> sendPayLoads(mParties), recvPayLoads(mParties);

	for (u64 i = 0; i < setSize; ++i)
	{
		set[i] = mSet2[i];
	}

	for (u64 idxP = 0; idxP < mParties; ++idxP)
	{
		sendPayLoads[idxP].resize(setSize);
		recvPayLoads[idxP].resize(setSize);
		for (u64 i = 0; i < setSize; ++i)
			sendPayLoads[idxP][i] = prng.get<block>();
	}

	std::string name("psi");
	BtIOService ios(0);

	int btCount = mParties;
	std::vector<BtEndpoint> ep(mParties);

	for (u64 i = 0; i < mParties; ++i)
	{
		dummy[i] = myIdx * 10 + i;
		if (i < myIdx)
		{
			u32 port = i * 10 + myIdx;//get the same port; i=1 & pIdx=2 =>port=102
			ep[i].start(ios, "localhost", port, false, name); //channel bwt i and pIdx, where i is sender
		}
		else if (i >myIdx)
		{
			u32 port = myIdx * 10 + i;//get the same port; i=2 & pIdx=1 =>port=102
			ep[i].start(ios, "localhost", port, true, name); //channel bwt i and pIdx, where i is receiver
		}
	}


	std::vector<std::vector<Channel*>> chls(mParties);

	for (u64 i = 0; i < mParties; ++i)
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

	std::vector<KkrtNcoOtReceiver> otRecv(mParties);
	std::vector<KkrtNcoOtSender> otSend(mParties);

	std::vector<OPPRFSender> send(mParties - myIdx);
	std::vector<OPPRFReceiver> recv(myIdx);
	binSet bins;

	std::mutex printMtx1, printMtx2;
	std::vector<std::thread>  pThrds(mParties);

	
	//##########################
	//### Offline Phasing
	//##########################

	bins.init(myIdx, mParties, setSize, psiSecParam,opt);
	u64 otCountRecv = bins.mCuckooBins.mBins.size();
	u64 otCountSend = bins.mSimpleBins.mBins.size();


	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
	{
		pThrds[pIdx] = std::thread([&, pIdx]() {
			if (pIdx < myIdx) {
				//I am a receiver if other party idx < mine
				recv[myIdx].init(opt, mParties, setSize, psiSecParam, bitSize, chls[pIdx], otCountRecv,otRecv[pIdx], otSend[pIdx], ZeroBlock);
			}
			else if (pIdx > myIdx) {
				send[myIdx].init(opt, mParties, setSize, psiSecParam, bitSize, chls[pIdx], otCountSend, otSend[pIdx], otRecv[pIdx], prng.get<block>());
			}
		});
	}

	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
		pThrds[pIdx].join();


	//std::cout << IoStream::lock;
	//if (myIdx == 0)
	//{
	//	Log::out << otSend[2].mGens[0].get<block>() << Log::endl;
	//	Log::out << otRecv[2].mGens[0][0].get<block>() << Log::endl;
	//	Log::out << otRecv[2].mGens[0][1].get<block>() << Log::endl;
	//	Log::out << "------------" << Log::endl;
	//}
	//if (myIdx == 2)
	//{
	//	Log::out << otSend[0].mGens[0].get<block>() << Log::endl;
	//	Log::out << otRecv[0].mGens[0][0].get<block>() << Log::endl;
	//	Log::out << otRecv[0].mGens[0][1].get<block>() << Log::endl;
	//}
	//std::cout << IoStream::unlock;

	//##########################
	//### Hashing
	//##########################
	bins.hashing2Bins(set, mParties);


	//##########################
	//### Online Phasing - compute OPRF
	//##########################

	//pThrds.clear();
	//pThrds.resize(mParties);
	//for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
	//{
	//	pThrds[pIdx] = std::thread([&, pIdx]() {
	//		if (pIdx < myIdx) {
	//			//I am a receiver if other party idx < mine
	//			recv[myIdx].recvSSTableBased(myIdx, bins,recvPayLoads[pIdx], chls[pIdx]);
	//			//	recv[pIdx].sendSSTableBased(pIdx, bins, sendPayLoads[pIdx], chls[pIdx]);
	//			}
	//		else if (pIdx > myIdx) {
	//			send[myIdx].sendSSTableBased(myIdx, bins, sendPayLoads[pIdx], chls[pIdx]);
	//		}
	//	});
	//}

	//for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
	//	pThrds[pIdx].join();



	//pThrds.clear();
	//pThrds.resize(mParties);
	//for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
	//{
	//	pThrds[pIdx] = std::thread([&, pIdx]() {
	//		if (pIdx < myIdx) {
	//			//I am a receiver if other party idx < mine
	//			recv[myIdx].recvSSTableBased(myIdx, bins, recvPayLoads[pIdx], chls[pIdx]);
	//			//	recv[pIdx].sendSSTableBased(pIdx, bins, sendPayLoads[pIdx], chls[pIdx]);
	//		}
	//		else if (pIdx > myIdx) {
	//			send[myIdx].sendSSTableBased(myIdx, bins, sendPayLoads[pIdx], chls[pIdx]);
	//		}
	//	});
	//}

	//for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
	//	pThrds[pIdx].join();


	//bins.hashing2Bins(set, mParties);
	//bins.mSimpleBins.print(myIdx, true, false, false, false);
	//bins.mCuckooBins.print(myIdx, true, false, false);

	//for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
	//{
	//	pThrds[pIdx] = std::thread([&, pIdx]() {
	//		if (pIdx < myIdx) {
	//			//I am a receiver if other party idx < mine
	//			recv[pIdx].init(opt, mParties, setSize, psiSecParam, bitSize, chls[pIdx], otRecv[pIdx], otSend[pIdx], ZeroBlock);
	//		}
	//		else if (pIdx > myIdx) {
	//			send[pIdx].init(opt, mParties, setSize, psiSecParam, bitSize, chls[pIdx], otSend[pIdx], otRecv[pIdx], prng.get<block>());
	//		}
	//	});
	//}

	/*for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
		pThrds[pIdx].join();*/


	for (u64 i = 0; i < mParties; ++i)
	{
		if (i != myIdx)
		{
			for (u64 j = 0; j < numThreads; ++j)
			{
				chls[i][j]->close();
			}
		}
	}

	for (u64 i = 0; i < mParties; ++i)
	{
		if (i != myIdx)
			ep[i].stop();
	}


	ios.stop();
}

void nChannel_party_test(u64 myIdx)
{
	u64 setSize = 1 << 5, psiSecParam = 40, bitSize = 128, numThreads = 1;
	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));


	std::vector<u8> dummy(mParties);
	std::vector<u8> revDummy(mParties);


	std::string name("psi");
	BtIOService ios(0);

	int btCount = mParties;
	std::vector<BtEndpoint> ep(mParties);

	for (u64 i = 0; i < mParties; ++i)
	{
		dummy[i] = myIdx * 10 + i;
		if (i < myIdx)
		{
			u32 port = i * 10 + myIdx;//get the same port; i=1 & pIdx=2 =>port=102
			ep[i].start(ios, "localhost", port, false, name); //channel bwt i and pIdx, where i is sender
		}
		else if (i >myIdx)
		{
			u32 port = myIdx * 10 + i;//get the same port; i=2 & pIdx=1 =>port=102
			ep[i].start(ios, "localhost", port, true, name); //channel bwt i and pIdx, where i is receiver
		}
	}


	std::vector<std::vector<Channel*>> chls(mParties);

	for (u64 i = 0; i < mParties; ++i)
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
	std::vector<std::thread>  pThrds(mParties);
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




	for (u64 i = 0; i < mParties; ++i)
	{
		if (i != myIdx)
		{
			for (u64 j = 0; j < numThreads; ++j)
			{
				chls[i][j]->close();
			}
		}
	}

	for (u64 i = 0; i < mParties; ++i)
	{
		if (i != myIdx)
			ep[i].stop();
	}


	ios.stop();
}

void nOPPRFn_EmptrySet_Test_Impl()
{
	u64 setSize = 1 << 5, psiSecParam = 40, bitSize = 128;
	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
	mSet2.resize(setSize);
	for (u64 i = 0; i < setSize; ++i)
	{
		mSet2[i] = prng.get<block>();
	}
	std::vector<std::thread>  pThrds(mParties);
	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
	{
		pThrds[pIdx] = std::thread([&, pIdx]() {
		//	Channel_party_test(pIdx);
			nParty(pIdx);
		});
	}
	for (u64 pIdx = 0; pIdx < pThrds.size(); ++pIdx)
		pThrds[pIdx].join();


}

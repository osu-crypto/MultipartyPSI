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


using namespace osuCrypto;
#define PRINT

void testPointer(std::vector<block>* test)
{
	//int length = test->size();
	//std::cout << length << std::endl;
	

	AES ncoInputHasher;
	
		ncoInputHasher.setKey(_mm_set1_epi64x(112434));
		ncoInputHasher.ecbEncBlocks((*test).data() , test->size() - 1, (*test).data() );
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
	u64 setSize = 1<<4;
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
		std::cout <<testSet[i] << std::endl;
	}

	block test = ZeroBlock;
	test.m128i_i8[0] = 31;
	BitPosition b;
	b.init(5);
	for (int i = 0; i < 3; ++i) b.mPos.insert(i);
	b.mPos.insert(6);
	b.mPos.insert(7);
	//std::cout << static_cast<int16_t>(b.map(test)) <<std::endl;
	//std::cout << static_cast<int16_t>(b.map2(test));


	BitPosition b2;
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

void hashing2Bins_Test_Impl()
{
	u64 setSize = 1 << 5, psiSecParam = 40, bitSize = 128, numParties = 2;
	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));

	std::vector<block> set(setSize);

	for (u64 i = 0; i < setSize; ++i)
	{
		set[i] = prng.get<block>();
	}

	binSet bins;

	bins.init(0, 2, setSize, psiSecParam);
	bins.hashing2Bins(set, 2);

	bins.mSimpleBins.print(1, true, false, false, false);
	bins.mCuckooBins.print(1, true, false, false);



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
	
	BitPosition b;
	b.init(5,5);
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
			Log::out << static_cast<int16_t>(b.mPos[j])<<" ";
	}
	Log::out << Log::endl;

	for (u64 j = 0; j < b.mMaps.size(); ++j)
	{
		Log::out << testSet[j] << " " << static_cast<int16_t>(b.mMaps[j]) << " " << Log::endl;
	}
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

    hashMap0.init(setSize, 40,1, true);
    hashMap1.init(setSize, 40,1, true);


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
			if(i!=j)
				ep[i*numParties+j].stop();
		}
	}

	ios.stop();
}
void OPPRF_EmptrySet_Test_Impl()
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
    std::thread thrd([&]() {


		send.init(numParties,setSize, psiSecParam, bitSize, sendChl, otSend0, otRecv1, prng.get<block>());
	//	send.hash2Bins(sendSet, sendChl);
		//send.getOPRFKeys(1,sendChl);
		//send.sendSecretSharing(1, sendPayLoads, sendChl);
	//	send.revSecretSharing(1, recvPayLoads, sendChl);
		//Log::out << "send.mSimpleBins.print(true, false, false,false);" << Log::endl;
	//	send.mSimpleBins.print(1,true, true, true, true);
		//Log::out << "send.mCuckooBins.print(true, false, false);" << Log::endl;
		//send.mCuckooBins.print(1,true, true, false);
	


		
		//send.mBins.print();

		//for (u64 i = 1; i < 3; ++i)
		//{
		//	Log::out << "Sender Bin#: " << i << " ";
		//	for (u64 j = 1; j < send.mBins.mBins[i].mIdx.size(); ++j)
		//	{
		//		Log::out << send.mBins.mBins[i].mIdx[j] << " - ";
		//		Log::out << send.mBins.mBins[i].mValOPRF[j] << Log::endl;
		//	}
		//}
		//
		//

    });
	recv.init(numParties,setSize, psiSecParam, bitSize, recvChl, otRecv0, otSend1, ZeroBlock);
	//recv.hash2Bins(recvSet, recvChl);
//	recv.getOPRFkeys(0, recvChl);
	//recv.revSecretSharing(0, recvPayLoads, recvChl);
//	recv.sendSecretSharing(0, sendPayLoads, recvChl);

	Log::out << "recv.mCuckooBins.print(true, false, false);" << Log::endl;
//	recv.mCuckooBins.print(0,true, true, false);
	
	//Log::out << "recv.mSimpleBins.print(true, false, false,false);" << Log::endl;
	//recv.mSimpleBins.print(0,true, true, true, true);

	/*recv.sendInput(recvSet, recvChl);
	recv.mBins.print();
*/

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

	thrd.join();

	

   



    sendChl[0]->close();
    recvChl[0]->close();

    ep0.stop();
    ep1.stop();
    ios.stop();
}

void OPPRFn_EmptrySet_Test_Impl()
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

	std::vector<KkrtNcoOtReceiver> otRecv(2);
	std::vector<KkrtNcoOtSender> otSend(2);

	std::vector<OPPRFSender> send(1);
	std::vector<OPPRFReceiver> recv(1);

	std::vector<binSet> bins(2);
	


	std::thread thrd([&]() {
		
		send[0].init(numParties, setSize, psiSecParam, bitSize, sendChl, otSend[0], otRecv[1], prng.get<block>());
		
		
		bins[0].init(0, numParties, setSize, psiSecParam);
		bins[0].hashing2Bins(sendSet, 2);
		//send.hash2Bins(sendSet, sendChl);
		send[0].getOPRFKeys(1, bins[0],sendChl);
		send[0].sendSecretSharing(1, bins[0], sendPayLoads, sendChl);
		//send.revSecretSharing(1, recvPayLoads, sendChl);
		//Log::out << "send.mSimpleBins.print(true, false, false,false);" << Log::endl;
		bins[0].mSimpleBins.print(1, true, true, true, true);
		//Log::out << "send.mCuckooBins.print(true, false, false);" << Log::endl;
		//send.mCuckooBins.print(1,true, true, false);




		//send.mBins.print();

		//for (u64 i = 1; i < 3; ++i)
		//{
		//	Log::out << "Sender Bin#: " << i << " ";
		//	for (u64 j = 1; j < send.mBins.mBins[i].mIdx.size(); ++j)
		//	{
		//		Log::out << send.mBins.mBins[i].mIdx[j] << " - ";
		//		Log::out << send.mBins.mBins[i].mValOPRF[j] << Log::endl;
		//	}
		//}
		//
		//

	});

	
	recv[0].init(numParties, setSize, psiSecParam, bitSize, recvChl, otRecv[0], otSend[1], ZeroBlock);
	
	bins[1].init(1, numParties, setSize, psiSecParam);
	bins[1].hashing2Bins(recvSet, 2);

	//recv.hash2Bins(recvSet, recvChl);
	recv[0].getOPRFkeys(0, bins[1], recvChl);
	recv[0].revSecretSharing(0, bins[1], recvPayLoads, recvChl);
	//recv.sendSecretSharing(0, sendPayLoads, recvChl);

	Log::out << "recv.mCuckooBins.print(true, false, false);" << Log::endl;
	bins[1].mCuckooBins.print(0, true, true, true);

	//Log::out << "recv.mSimpleBins.print(true, false, false,false);" << Log::endl;
	//recv.mSimpleBins.print(0,true, true, true, true);

	/*recv.sendInput(recvSet, recvChl);
	recv.mBins.print();
	*/

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
	Log::out << otSend[0].mT.size()[1] << Log::endl;
	Log::out << otSend[1].mT.size()[1] << Log::endl;
	Log::out << otSend[0].mGens[0].get<block>() << Log::endl;
	Log::out << otRecv[0].mGens[0][0].get<block>() << Log::endl;
	Log::out << otRecv[0].mGens[0][1].get<block>() << Log::endl;
	Log::out << "------------" << Log::endl;
	Log::out << otSend[1].mGens[0].get<block>() << Log::endl;
	Log::out << otRecv[1].mGens[0][0].get<block>() << Log::endl;
	Log::out << otRecv[1].mGens[0][1].get<block>() << Log::endl;
	std::cout << IoStream::unlock;

#endif

	thrd.join();







	sendChl[0]->close();
	recvChl[0]->close();

	ep0.stop();
	ep1.stop();
	ios.stop();
}


#if 0
void OPPRF_FullSet_Test_Impl()
{
    setThreadName("CP_Test_Thread");
    u64 setSize = 8, psiSecParam = 40, numThreads(1), bitSize = 128;
    PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));


    std::vector<block> sendSet(setSize), recvSet(setSize);
    for (u64 i = 0; i < setSize; ++i)
    {
        sendSet[i] = recvSet[i] = prng.get<block>();
    }

    std::shuffle(sendSet.begin(), sendSet.end(), prng);


    std::string name("psi");

    BtIOService ios(0);
    BtEndpoint ep0(ios, "localhost", 1212, true, name);
    BtEndpoint ep1(ios, "localhost", 1212, false, name);


    std::vector<Channel*> sendChls(numThreads), recvChls(numThreads);
    for (u64 i = 0; i < numThreads; ++i)
    {
        sendChls[i] = &ep1.addChannel("chl" + std::to_string(i), "chl" + std::to_string(i));
        recvChls[i] = &ep0.addChannel("chl" + std::to_string(i), "chl" + std::to_string(i));
    }


    KkrtNcoOtReceiver otRecv;
    KkrtNcoOtSender otSend;

    OPPRFSender send;
	OPPRFReceiver recv;
    std::thread thrd([&]() {


        send.init(setSize, psiSecParam, bitSize, sendChls, otSend, prng.get<block>());
       // send.sendInput(sendSet, sendChls);
    });

    recv.init(setSize, psiSecParam, bitSize, recvChls, otRecv, ZeroBlock);
   // recv.sendInput(recvSet, recvChls);


   /* if (recv.mIntersection.size() != setSize)
        throw UnitTestFail();*/

    thrd.join();

    for (u64 i = 0; i < numThreads; ++i)
    {
        sendChls[i]->close();
        recvChls[i]->close();
    }

    ep0.stop();
    ep1.stop();
    ios.stop();

}

void OPPRF_SingltonSet_Test_Impl()
{
    setThreadName("Sender");
    u64 setSize = 128, psiSecParam = 40, bitSize = 128;

    PRNG prng(_mm_set_epi32(4253465, 34354565, 234435, 23987045));

    std::vector<block> sendSet(setSize), recvSet(setSize);
    for (u64 i = 0; i < setSize; ++i)
    {
        sendSet[i] = prng.get<block>();
        recvSet[i] = prng.get<block>();
    }

    sendSet[0] = recvSet[0];

    std::string name("psi");
    BtIOService ios(0);
    BtEndpoint ep0(ios, "localhost", 1212, true, name);
    BtEndpoint ep1(ios, "localhost", 1212, false, name);


    Channel& recvChl = ep1.addChannel(name, name);
    Channel& sendChl = ep0.addChannel(name, name);

    KkrtNcoOtReceiver otRecv;
    KkrtNcoOtSender otSend;

    OPPRFSender send;
    OPPRFReceiver recv;
    std::thread thrd([&]() {


        send.init(setSize, psiSecParam, bitSize, sendChl, otSend, prng.get<block>());
        //send.sendInput(sendSet, sendChl);
    });

    recv.init(setSize, psiSecParam, bitSize, recvChl, otRecv, ZeroBlock);
   // recv.sendInput(recvSet, recvChl);

    thrd.join();

    /*if (recv.mIntersection.size() != 1 ||
        recv.mIntersection[0] != 0)
        throw UnitTestFail();*/


    //std::cout << gTimer << std::endl;

    sendChl.close();
    recvChl.close();

    ep0.stop();
    ep1.stop();
    ios.stop();
}
#endif
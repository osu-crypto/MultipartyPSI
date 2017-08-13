#include "PM_Tests.h"

#include "Common.h"
#include "Network/BtEndpoint.h"
#include "Common/Defines.h"
#include "OPPRF/OPPRFReceiver.h"
#include "OPPRF/OPPRFSender.h"
#include "OPPRF/binSet.h"
#include "Common/Log.h"
#include "Common/Log.h"
#include <memory>
#include "Common/Timer.h"
#include "Crypto/Curve.h"

#define PARALLEL

#include "Common/ByteStream.h"
#include "Common/BitVector.h"
#include "Crypto/sha1.h"

#include "TwoChooseOne/IknpOtExtReceiver.h"
#include "TwoChooseOne/IknpOtExtSender.h"

#include "TwoChooseOne/KosOtExtReceiver.h"
#include "TwoChooseOne/KosOtExtSender.h"

#include "TwoChooseOne/LzKosOtExtReceiver.h"
#include "TwoChooseOne/LzKosOtExtSender.h"

#include "NChooseOne/KkrtNcoOtReceiver.h"
#include "NChooseOne/KkrtNcoOtSender.h"

#include "NChooseOne/KkrtNcoOtReceiver.h"
#include "NChooseOne/KkrtNcoOtSender.h"


#include "NChooseOne/Oos/OosNcoOtReceiver.h"
#include "NChooseOne/Oos/OosNcoOtSender.h"

#include "Hashing/CuckooHash.h"
#include "Hashing/Hints.h"
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
#include "Tools/Tools.h"

using namespace osuCrypto;
#define PRINT
//#define BIN_PRINT


void PM_Test_Impl()
{

	std::string name("psi");
	BtIOService ios(0);

	BtEndpoint ep0(ios, "127.0.0.1", 1212, true, "ep");
	BtEndpoint ep1(ios, "127.0.0.1", 1212, false, "ep");
	Channel& senderChannel = ep1.addChannel("chl", "chl");
	Channel& recvChannel = ep0.addChannel("chl", "chl");

	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
	u64 numBlkP=8, numBlkT=10, blkSize=128;
	u64 numP = numBlkP*blkSize, numT = numBlkT*blkSize, numOTs= numP;
	std::vector<block>text(numBlkT);
	for (u64 i = 0; i < text.size(); i++)
		text[i]= prng.get<block>();

	BitVector parttern(numP);
		
	//1-o-o-3
	//   2pattern x  text
	// #01---------------r => if p[0]=1 gets r, p[0]=1 does r+1
	// #*---------------r+text
	// #01---------------
	// #1---------------
	// #*---------------

	/*std::vector<std::vector<block>> matrix(2* numP);
	
	for (u64 i = 0; i < matrix.size(); i+=2)
	{
		matrix[i].resize(numBlkT);
		for (u64 j = 0; j < text.size(); j++)
			matrix[i][j] = prng.get<block>();

		matrix[i+1].resize(numBlkT);
		for (u64 j = 0; j < text.size(); j++)
			matrix[i+1][j] = matrix[i][j]^text[j];
	}*/

	PRNG prng0(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
	PRNG prng1(_mm_set_epi32(4253233465, 334565, 0, 235));

	//t1, t1+s1
	std::vector<block> recvMsg(numOTs), baseRecv(128);
	std::vector<std::array<block, 2>> sendMsg(numOTs), baseSend(128);
	BitVector choices(numOTs), baseChoice(128);
	choices.randomize(prng0);
	baseChoice.randomize(prng0);	
		
	IknpOtExtSender sender;
	IknpOtExtReceiver recv;

	std::vector<std::vector<block>> matrixOffline(2 * numBlkP);
	std::vector<std::vector<block>>partternOffline(numBlkP);

	std::vector<std::vector<block>> matrixOnline(2 * numBlkP);
	std::vector<std::vector<block>>partternOnline(numBlkP);

	std::vector<PRNG> genRecv(numOTs);
	std::vector<std::array<PRNG, 2>> genSend(numOTs);
			

	std::thread thrd = std::thread([&]() {
		Log::setThreadName("pattern"); 
		
//Offline
		NaorPinkas base;
		base.send(baseSend, prng, senderChannel, 2);
		recv.setBaseOts(baseSend);
		recv.receive(choices, recvMsg, prng, recvChannel);
		for (u64 i = 0; i < genRecv.size(); i++)
		{
			genRecv[i].SetSeed(recvMsg[i]);
		}

		for (u64 i = 0; i < partternOffline.size(); i++)
		{
			partternOffline[i].resize(numBlkT);
			for (u64 j = 0; j < partternOffline[i].size(); j++)
				partternOffline[i][j] = genRecv[i].get<block>();
		}

	});
	

	NaorPinkas base;
	base.receive(baseChoice, baseRecv, prng, recvChannel, 1);
	sender.setBaseOts(baseRecv, baseChoice);
	sender.send(sendMsg, prng1, senderChannel);
	for (u64 i = 0; i < genSend.size(); i++)
	{
		genSend[i][0].SetSeed(sendMsg[i][0]);
		genSend[i][1].SetSeed(sendMsg[i][1]);
	}

	for (u64 i = 0; i < matrixOffline.size(); i += 2)
	{
		matrixOffline[i].resize(numBlkT);
		matrixOffline[i + 1].resize(numBlkT);
		for (u64 j = 0; j < matrixOffline[i].size(); j++)
		{
			matrixOffline[i][j] = genSend[i][0].get<block>();
			matrixOffline[i + 1][j] = genSend[i][1].get<block>();
		}
	}


	thrd.join();

	//online => send correction vals 1024x128
	u64 step = 1024;

	
	
	 thrd = std::thread([&]() {
		Log::setThreadName("pattern");
		
		for (u64 k = 0; k < numBlkT; k++)
		{
			for (u64 i = 0; i < numP; i += step)
			{
				uPtr<Buff> recvMaskBuff(new Buff);
				recvMaskBuff->resize(step * sizeof(block));
				auto maskBFView = recvMaskBuff->getArrayView<block>();
				for (u64 j = 0; j < maskBFView.size(); j++)
				{
				//	if (parttern[i])
				}
			}
		}

	});

	//semd 1024x128 each time. We have numPxnumBlkT in total
	//send row by row
	
	for (u64 k = 0; k < numBlkT; k++) 
	{
		for (u64 i = 0; i < numP; i += step)
		{
			uPtr<Buff> sendMaskBuff(new Buff);
			sendMaskBuff->resize(step * sizeof(block));
			auto maskBFView = sendMaskBuff->getArrayView<block>();
			for (u64 j = 0; j < maskBFView.size(); j++)
			{
				//compute H(q)+H(q+s)+m
				maskBFView[i + j] = matrixOffline[i + j][k] ^ matrixOffline[i + j + 1][k] ^ text[k];
			}
			senderChannel.asyncSend(std::move(sendMaskBuff));
		}
	}
	thrd.join();



	

	

	
	

	std::cout << matrixOffline[2][0] << std::endl;
	std::cout << matrixOffline[2][1] << std::endl;
	//std::cout << partternOffline[2] << std::endl;

	/*std::cout << sendMsg[199][0] << std::endl;
	std::cout << sendMsg[199][1] << std::endl;
	std::cout << sendMsg[199][2] << std::endl;
	std::cout << sendMsg[199][3] << std::endl;
	
	std::cout << recvMsg[199] << std::endl;

	std::cout << choices[0][199] << std::endl;
	std::cout << choices[1][199] << std::endl;
*/
	block a = sendMsg[100][0] ^ sendMsg[100][1];
	block b = sendMsg[150][0] ^ sendMsg[150][1];
	std::cout << a << std::endl;
	std::cout << b << std::endl;

	block delta = *(block*)baseChoice.data();

	std::cout << delta << std::endl;

#if 0
	auto& params = k233;
	auto seed = prng.get<block>();
	EllipticCurve curve(params, seed);

	EccNumber
		alpha(curve, prng),//(mainPk->get_rnd_num()),
						   //PKr(curve),//(mainPk->get_num()),
		tmp(curve);

	EccNumber beta(curve, prng);

	const EccPoint&
		g = curve.getGenerator();

	EccPoint pA(curve);
	EccPoint pB(curve);

	pA = g*alpha;

	u8 choice = choices[0];

	if (!choice)
		pB = pA + g*beta;
	else
		pB = g*beta;

	EccPoint pPKr(curve);//(thrdPK->get_fe()),
	pPKr = pA*beta;

	EccPoint
		pPK0(curve),//(thrdPK->get_fe()),
		pPK1(curve);//(thrdPK->get_fe()),

	pPK0 = pB*alpha;
	pPK1 = (pB-pA)*alpha;

	std::cout << "pPK0: " << pPK0 << std::endl;
	std::cout << "pPK1: " << pPK1 << std::endl;
	std::cout << "pPKr: " << pPKr << std::endl;
#endif 


	senderChannel.close();
	recvChannel.close();

	ep1.stop();
	ep0.stop();

	ios.stop();
}


void IknpOtExt4_Test_Impl()
{
	setThreadName("Sender");

	BtIOService ios(0);
	BtEndpoint ep0(ios, "127.0.0.1", 1212, true, "ep");
	BtEndpoint ep1(ios, "127.0.0.1", 1212, false, "ep");
	Channel& senderChannel = ep1.addChannel("chl", "chl");
	Channel& recvChannel = ep0.addChannel("chl", "chl");

	PRNG prng0(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
	PRNG prng1(_mm_set_epi32(4253233465, 334565, 0, 235));

	u64 numOTs = 200;

	std::vector<block> recvMsg(numOTs), baseRecv(128);
	std::vector<std::array<block, 2>> sendMsg(numOTs), baseSend(128);
	BitVector choices(numOTs), baseChoice(128);
	choices.randomize(prng0);
	baseChoice.randomize(prng0);

	prng0.get((u8*)baseSend.data()->data(), sizeof(block) * 2 * baseSend.size());
	for (u64 i = 0; i < 128; ++i)
	{
		baseRecv[i] = baseSend[i][baseChoice[i]];
	}

	IknpOtExtSender sender;
	IknpOtExtReceiver recv;

	std::thread thrd = std::thread([&]() {



		recv.setBaseOts(baseSend);
		recv.receive(choices, recvMsg, prng0, recvChannel);
	});



	//{
	//    std::lock_guard<std::mutex> lock(mMtx);
	//    for (u64 i = 0; i < baseOTs.receiver_outputs.size(); ++i)
	//    {
	//        std::cout << "i  " << baseOTs.receiver_outputs[i] << " " << (int)baseOTs.receiver_inputs[i] << std::endl;
	//    }
	//}
	sender.setBaseOts(baseRecv, baseChoice);
	sender.send(sendMsg, prng1, senderChannel);
	thrd.join();

	//for (u64 i = 0; i < baseOTs.receiver_outputs.size(); ++i)
	//{
	//    std::cout << sender.GetMessage(i, 0) << " " << sender.GetMessage(i, 1) << "\n" << recv.GetMessage(1) << "  " << recv.mChoiceBits[i] << std::endl;
	//}
//	OT_100Receive_Test(choices, recvMsg, sendMsg);




	senderChannel.close();
	recvChannel.close();


	ep1.stop();
	ep0.stop();

	ios.stop();

	//senderNetMgr.Stop();
	//recvNetMg
}




#include <iostream>

using namespace std;
#include "Common/Defines.h"
#include "PSI/BopPsiReceiver.h"
#include "PSI/BopPsiSender.h"
#include "Network/BtEndpoint.h" 
#include <math.h>
#include "Common/Log.h"
#include "Common/Timer.h"
#include "Crypto/PRNG.h"
using namespace bOPRF;
#include <fstream>



void senderGetLatency(Channel& chl)
{
	u8 dummy[1];
	chl.asyncSend(dummy, 1);
	chl.recv(dummy, 1);
	chl.asyncSend(dummy, 1);
}

void recverGetLatency(Channel& chl)
{
	u8 dummy[1];
	chl.recv(dummy, 1);
	Timer timer;
	auto start = timer.setTimePoint("");
	chl.asyncSend(dummy, 1);
	chl.recv(dummy, 1);
	auto end = timer.setTimePoint("");
	Log::out << "latency: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << Log::endl;
}

void pingTest(Channel& chl, bool sender)
{
	u64 count = 100;
	std::array<u8, 131072 / 100> oneMB;

	Timer timer;
	ByteStream buff;
	if (sender)
	{
		auto send = timer.setTimePoint("ping sent");
		for (u64 i = 0; i < count; ++i)
		{
			chl.asyncSend("c", 1);
			chl.recv(buff);
			if (buff.size() != 1)
			{
				Log::out << std::string((char*)buff.data(), (char*)buff.data() + buff.size()) << Log::endl;
				throw std::runtime_error("");
			}
		}
		chl.asyncSend("r", 1);

		auto recv = timer.setTimePoint("ping recv");

		auto ping = std::chrono::duration_cast<std::chrono::microseconds>(recv - send).count() / count;

		Log::out << "ping " << ping << " us" << Log::endl;

		send = timer.setTimePoint("");
		chl.asyncSend(oneMB.data(), oneMB.size());
		chl.recv(buff);
		recv = timer.setTimePoint("");
		if (buff.size() != 1) throw std::runtime_error("");

		double time = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(recv - send).count() - ping);

		chl.recv(buff);
		chl.asyncSend("r", 1);
		if (buff.size() != oneMB.size()) throw std::runtime_error("");


		Log::out << (8000000 / time) << " Mbps" << Log::endl;
	}
	else
	{
		chl.recv(buff);

		auto send = timer.setTimePoint("ping sent");
		for (u64 i = 0; i < count; ++i)
		{
			chl.asyncSend("r", 1);
			chl.recv(buff);
			if (buff.size() != 1) throw std::runtime_error("");

		}

		auto recv = timer.setTimePoint("ping recv");

		auto ping = std::chrono::duration_cast<std::chrono::microseconds>(recv - send).count() / count;
		std::cout << "ping " << ping << " us" << std::endl;

		chl.recv(buff);
		chl.asyncSend("r", 1);
		if (buff.size() != oneMB.size()) throw std::runtime_error("");


		send = timer.setTimePoint("");
		chl.asyncSend(oneMB.data(), oneMB.size());
		chl.recv(buff);
		recv = timer.setTimePoint("");
		if (buff.size() != 1) throw std::runtime_error("");

		double time = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(recv - send).count() - ping);

		Log::out << (8000000 / time) << " Mbps" << Log::endl;

	}

}

void BopSender()
{
	Log::out << "BopSender()" << Log::endl;
	u64 numThreads = 1;
	u64 numTrial(10);

	std::fstream online, offline;

	Log::out << "role  = sender (" << numThreads << ") SSOtPSI" << Log::endl;

	// , numThreads(1);


	std::string name("psi");

	BtIOService ios(0);
	BtEndpoint ep0(ios, "localhost", 1215, true, name);


	std::vector<Channel*> sendChls(numThreads);
	for (u64 i = 0; i < numThreads; ++i)
		sendChls[i] = &ep0.addChannel(name + std::to_string(i), name + std::to_string(i));

	senderGetLatency(*sendChls[0]);

	pingTest(*sendChls[0], true);

	for (auto pow : { 8,12,16,20,24 })
	{

		u64 setSize = (1 << pow), psiSecParam = 40;
		u64 offlineTimeTot(0);
		u64 onlineTimeTot(0);

		for (u64 j = 0; j < numTrial; ++j)
		{
			//u64 repeatCount = 4;
			u64 setSize = (1 << pow), psiSecParam = 40;
			PRNG prngSame(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
			//	PRNG prngSame2(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
			PRNG prngDiff(_mm_set_epi32(43465, 32254, 2435, 2398045));

			std::vector<block> sendSet(setSize);

			u64 rand = prngSame.get_u32() % setSize;

			for (u64 i = 0; i < rand; ++i)
			{
				sendSet[i] = prngSame.get_block();
			}
			for (u64 i = rand; i < setSize; ++i)
			{
				sendSet[i] = prngDiff.get_block();
			}
			//std::shuffle(sendSet.begin(), sendSet.end(), prngSame);
			SSOtExtSender OTSender0;


			BopPsiSender sendPSIs;

			u8 dumm[1];
			sendChls[0]->asyncSend(dumm, 1);
			sendChls[0]->recv(dumm, 1);
			sendChls[0]->asyncSend(dumm, 1);


			gTimer.reset();
			sendPSIs.init(setSize, psiSecParam, *sendChls[0], OTSender0, OneBlock);


			sendPSIs.sendInput(sendSet, sendChls);
			//Log::out << "threads =  " << numThreads << Log::endl << gTimer << Log::endl << Log::endl << Log::endl;

			u64 otIdx = 0;
		}
	}

	for (u64 i = 0; i < numThreads; ++i)
	{
		sendChls[i]->close();
	}
	//sendChl.close();
	//recvChl.close();

	ep0.stop();
	ios.stop();
}

void BopRecv()
{
	Log::out << "BopRecv()" << Log::endl;

	u64 numThreads = 1;
	//u64 repeatCount = 4;


	std::fstream online, offline, total;
	total.open("./output.txt", total.trunc | total.out);
	u64 numTrial(10);

	std::string name("psi");

	BtIOService ios(0);
	BtEndpoint ep1(ios, "localhost", 1215, false, name);


	std::vector<Channel*> recvChls(numThreads);
	for (u64 i = 0; i < numThreads; ++i)
		recvChls[i] = &ep1.addChannel(name + std::to_string(i), name + std::to_string(i));

	recverGetLatency(*recvChls[0]);
	pingTest(*recvChls[0], false);
	Log::out << "role  = recv (" << numThreads << ") SSOtPSI" << Log::endl;
	//8,12,16,
	Log::out << "--------------------------\n";

	for (auto pow : { 8,12,16,20,24 })
		//for (auto pow : { 16,20 })
	{

		u64 offlineTimeTot(0);
		u64 onlineTimeTot(0);
		u64 setSize = (1 << pow), psiSecParam = 40;

		Log::out << "setSize" << "  |  " << "offline(ms)" << "  |  " << "online(ms)" << Log::endl;

		for (u64 j = 0; j < numTrial; ++j)
		{
			PRNG prngSame(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
			PRNG prngDiff(_mm_set_epi32(434653, 23, 11, 56));

			std::vector<block> recvSet(setSize);
			u64 rand = prngSame.get_u32() % setSize;
			for (u64 i = 0; i < rand; ++i)
			{
				recvSet[i] = prngSame.get_block();
			}

			for (u64 i = rand; i < setSize; ++i)
			{
				recvSet[i] = prngDiff.get_block();
			}

			SSOtExtReceiver OTRecver0;
			BopPsiReceiver recvPSIs;			

			u8 dumm[1];
			recvChls[0]->recv(dumm, 1);
			recvChls[0]->asyncSend(dumm, 1);
			recvChls[0]->recv(dumm, 1);

			//gTimer.reset();
			Timer timer;
			timer.setTimePoint("start");
			auto start = timer.setTimePoint("start");
			recvPSIs.init(setSize, psiSecParam, recvChls, OTRecver0, ZeroBlock);
			auto mid = timer.setTimePoint("init");
			recvPSIs.sendInput(recvSet, recvChls);
			//timer.setTimePoint("Done");
			auto end = timer.setTimePoint("done");

			auto offlineTime = std::chrono::duration_cast<std::chrono::milliseconds>(mid - start).count();
			auto online = std::chrono::duration_cast<std::chrono::milliseconds>(end - mid).count();
			offlineTimeTot += offlineTime;
			onlineTimeTot += online;
			//output
			//Log::out << "#Output Intersection: " << recvPSIs.mIntersection.size() << Log::endl;
			//Log::out << "#Expected Intersection: " << rand << Log::endl;
			Log::out << setSize << "\t\t" << offlineTime << "\t\t" << online << Log::endl;

		}
		Log::out << "2^" << pow << "-- Online Avg Time: " << onlineTimeTot / numTrial << " ms " << "\n";
		Log::out << "2^" << pow << "-- Offline Avg Time: " << offlineTimeTot / numTrial << " ms " << "\n";
		Log::out << "2^" << pow << "-- Total Avg Time: " << (offlineTimeTot + onlineTimeTot) / numTrial << " ms " << "\n";
		Log::out << "--------------------------\n";

		total << "2^" << pow << "-- Online Avg Time: " << onlineTimeTot / numTrial << " ms " << "\n";
		total << "2^" << pow << "-- Offline Avg Time: " << offlineTimeTot / numTrial << " ms " << "\n";
		total << "2^" << pow << "-- Total Avg Time: " << (offlineTimeTot + onlineTimeTot) / numTrial << " ms " << "\n";
		total << "--------------------------\n";

	}


	for (u64 i = 0; i < numThreads; ++i)
	{
		recvChls[i]->close();
	}
	//sendChl.close();
	//recvChl.close();

	ep1.stop();
	ios.stop();
}


void BopTest()
{
	Log::out << "Test()" << Log::endl;

	u64 numThreads = 1;
	u64 setSize = (1 << 8), psiSecParam = 40;// , numThreads(1);

	//generate data
	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
	PRNG prngDiff1(_mm_set_epi32(434653, 23, 11, 56));
	PRNG prngDiff2(_mm_set_epi32(43465, 32254, 2435, 2398045));

	std::vector<block> sendSet(setSize), recvSet(setSize);

	u64 rand = setSize / 4;
	//same input value =>intersection
	for (u64 i = 0; i < rand; ++i)
	{
		sendSet[i] = prng.get_block();
		recvSet[i] = sendSet[i];
	}

	//different input value 
	for (u64 i = rand; i < setSize; ++i)
	{
		sendSet[i] = prngDiff1.get_block();
	}
	for (u64 i = rand; i < setSize; ++i)
	{
		recvSet[i] = prngDiff2.get_block();
	}

	//shuffle
	std::shuffle(sendSet.begin(), sendSet.end(), prng);
	std::shuffle(recvSet.begin(), recvSet.end(), prng);


	std::string name("psi");

	BtIOService ios(0);
	BtEndpoint ep0(ios, "localhost", 1213, true, name);
	BtEndpoint ep1(ios, "localhost", 1213, false, name);


	std::vector<Channel*> recvChls(numThreads), sendChls(numThreads);
	for (u64 i = 0; i < numThreads; ++i)
		recvChls[i] = &ep1.addChannel(name + std::to_string(i), name + std::to_string(i));
	for (u64 i = 0; i < numThreads; ++i)
		sendChls[i] = &ep0.addChannel(name + std::to_string(i), name + std::to_string(i));


	SSOtExtSender OTSender0;
	SSOtExtReceiver OTRecver0;

	auto bb = prng.get_block();

	BopPsiSender sendPSIs;
	BopPsiReceiver recvPSIs;

	std::thread thrd([&]() {
		PRNG prng(bb);
		//sender thread
		sendPSIs.init(setSize, psiSecParam, *sendChls[0], OTSender0, OneBlock);
		sendPSIs.sendInput(sendSet, sendChls);
	});

	u64 otIdx = 0;

	gTimer.reset();
	//receiver thread
	auto start = gTimer.setTimePoint("start");
	recvPSIs.init(setSize, psiSecParam, recvChls, OTRecver0, ZeroBlock);
	recvPSIs.sendInput(recvSet, recvChls);
	auto end = gTimer.setTimePoint("done");


	auto totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	Log::out << gTimer << Log::endl;

	Log::out << "Total Time: " << totalTime << "(ms)\t\t\n" << Log::endl;

	//output
	Log::out << "#Output Intersection: " << recvPSIs.mIntersection.size() << Log::endl;
	Log::out << "#Expected Intersection: " << rand << Log::endl;

	if (recvPSIs.mIntersection.size() != rand)
	{
		Log::out << "\nbad intersection,  expecting full set of size  " << setSize << " but got " << recvPSIs.mIntersection.size() << Log::endl;
		//throw std::runtime_error(std::string("bad intersection, "));
	}
	else
	{
		Log::out << "\nCool. Good PSI!" << Log::endl;
	}

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

void usage(const char* argv0)
{
	Log::out << "Error! Please use:" << Log::endl;
	Log::out << "\t 1. For unit test: " << argv0 << " -t" << Log::endl;
	Log::out << "\t 2. For simulation (2 terminal): " << Log::endl;;
	Log::out << "\t\t Sender terminal: " << argv0 << " -r 0" << Log::endl;
	Log::out << "\t\t Receiver terminal: " << argv0 << " -r 1" << Log::endl;
}

int main(int argc, char** argv)
{
	if (argc == 2 && argv[1][0] == '-' && argv[1][1] == 't') {
		BopTest();
	}
	else if (argc == 3 && argv[1][0] == '-' && argv[1][1] == 'r' && atoi(argv[2]) == 0) {
		BopSender();
	}
	else if (argc == 3 && argv[1][0] == '-' && argv[1][1] == 'r' && atoi(argv[2]) == 1) {
		BopRecv();
	}
	else {
		usage(argv[0]);
	}

	return 0;
}
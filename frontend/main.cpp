
#include <iostream>
#include "Network/BtChannel.h"
#include "Network/BtEndpoint.h"

using namespace std;
#include "UnitTests.h" 
#include "Common/Defines.h"
using namespace osuCrypto;

#include "bloomFilterMain.h"
#include "dcwMain.h"
#include "dktMain.h"
#include "OtBinMain.h"
#include "bitPosition.h"

#include "TwoChooseOne/KosOtExtReceiver.h"
#include "TwoChooseOne/KosOtExtSender.h"
#include <numeric>
#include "Common/Log.h"
//int miraclTestMain();

#include "cuckoo/cuckooTests.h"
void usage(const char* argv0)
{
	std::cout << "Error! Please use:" << std::endl;
	std::cout << "\t 1. For unit test: " << argv0 << " -t" << std::endl;
	std::cout << "\t 2. For simulation (3 parties <=> 3 terminals): " << std::endl;;
	std::cout << "\t\t terminal: " << argv0 << " -p pIdx" << std::endl;
}
int main(int argc, char** argv)
{
	//std::cout << "fffffffff\n";
	//myCuckooTest_stash();
	 //Bit_Position_Random_Test();
	//return 0;
	//OPPRF2_EmptrySet_Test_Main();
	//OPPRFn_EmptrySet_Test_Main();
	
	std::vector<block> mSet;
	 u64 setSize = 1 << 20, psiSecParam = 40, bitSize = 128;
	 PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
	 mSet.resize(setSize);
	 for (u64 i = 0; i < setSize; ++i)
	 {
		 mSet[i] = prng.get<block>();
	 }

	
  ////  if (argc == 2)
  ////  {
		////OPPRFSend();
		////BarkOPRSend();
		////party3(0, setSize,  mSet);

  ////  }
  ////  else if (argc == 3)
  ////  {
		////BarkOPRFRecv();
		////OPPRFRecv();
		////party3(1, setSize, mSet);
  ////  }
  ////  else if (argc == 4)
  ////  {
		////party3(2, setSize,mSet);
  ////  }


	if (argc == 3 && argv[1][0] == '-' && argv[1][1] == 'p' && atoi(argv[2]) == 0) {
		party3(0, setSize, mSet);
	}
	else if (argc == 3 && argv[1][0] == '-' && argv[1][1] == 'p' && atoi(argv[2]) == 1) {
		party3(1, setSize, mSet);
	}
	else if (argc == 3 && argv[1][0] == '-' && argv[1][1] == 'p' && atoi(argv[2]) == 2) {
		party3(2, setSize, mSet);
	}
	else {
		usage(argv[0]);
	}

    return 0;
}
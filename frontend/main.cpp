
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
	//OPPRF3_EmptrySet_Test_Main();
	//return 0;
	//OPPRFnt_EmptrySet_Test_Main();
	//OPPRFnt_EmptrySet_Test_Main();

	u64 trials=1;

	std::vector<block> mSet;
	 u64 setSize = 1 << 20, psiSecParam = 40, bitSize = 128;
	 u64 nParties, tParties;
	 u64 roundOPPRF;
	 PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
	 mSet.resize(setSize);
	 for (u64 i = 0; i < setSize; ++i)
		 mSet[i] = prng.get<block>();

	 if (argc == 7) {
		 if (argv[1][0] == '-' && argv[1][1] == 'n')
			 nParties = atoi(argv[2]);

		 
		 if (nParties == 3)
		 {
			 if (argv[3][0] == '-' && argv[3][1] == 'r')
				 roundOPPRF = atoi(argv[4]);

			 if (argv[5][0] == '-' && argv[5][1] == 'p') {
				 u64 pIdx = atoi(argv[6]);
				 if(roundOPPRF==1)
					party3(pIdx, setSize, trials);
				 else
					 tparty(pIdx, nParties, 2, setSize, trials);
			 }
		 }
		 else
		 {
			 if (argv[3][0] == '-' && argv[3][1] == 't')
				 tParties = atoi(argv[4]);

			 if (argv[5][0] == '-' && argv[5][1] == 'p') {
				 u64 pIdx = atoi(argv[6]);
				 std::cout << "pIdx:  " << pIdx << "\n";
				 tparty(pIdx, nParties, tParties, setSize, trials);
			 }
		 }
		 	 
	 }
	 else if (argc == 2) {
		 if (argv[1][0] == '-' && argv[1][1] == 'u')
		 {
			 OPPRFnt_EmptrySet_Test_Main();
		 }
	 }
	 else {
		 usage(argv[0]);
	 }
    return 0;
}
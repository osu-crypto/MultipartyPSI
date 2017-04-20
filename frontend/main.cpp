
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
	//OPPRFn_Aug_EmptrySet_Test_Impl();
	//return 0;

	u64 trials=1;

	std::vector<block> mSet;
	 u64 setSize = 1 << 10, psiSecParam = 40, bitSize = 128;
	 u64 nParties, tParties;
	 u64 roundOPPRF;
	 PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
	 mSet.resize(setSize);
	 for (u64 i = 0; i < setSize; ++i)
		 mSet[i] = prng.get<block>();

	
	 nParties = atoi(argv[1]);
	 u64 pIdx = atoi(argv[2]);
	 std::cout << "pIdx:  " << pIdx << "\n";
	 std::cout << "nParties:  " << nParties << "\n";

	 //TODO(remove this hash: unconditional zero-sharing
	 //only one time => very mirror effect on perfomance
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


	 aug_party(pIdx, nParties, mSet.size(), mSet, mPRNGSeeds[pIdx]);
	 return 0;

	 if (argc == 7) {
		 if (argv[1][0] == '-' && argv[1][1] == 'n')
			 nParties = atoi(argv[2]);

		 
		 if (nParties == 3)
		 {
			 if (argv[3][0] == '-' && argv[3][1] == 'r')
				 roundOPPRF = atoi(argv[4]);

			 if (argv[5][0] == '-' && argv[5][1] == 'p') {
				 u64 pIdx = atoi(argv[6]);
				 if (roundOPPRF == 1)
				 {
					// party3(pIdx, setSize, trials);
					 aug_party(pIdx, nParties, setSize, mSet, mPRNGSeeds[pIdx]);
				 }
				 else
				 {
					// tparty(pIdx, nParties, 2, setSize, trials);
					 std::cout << "aug_party";
					aug_party(pIdx, 2, mSet.size(), mSet, mPRNGSeeds[pIdx]);
				 }
			 
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
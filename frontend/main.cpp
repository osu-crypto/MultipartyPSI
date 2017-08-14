#include <iostream>
#include "Network/BtChannel.h"
#include "Network/BtEndpoint.h"

using namespace std;
#include "Common/Defines.h"
using namespace osuCrypto;

#include "PsiMain.h"

#include <numeric>
#include "Common/Log.h"

//#include "cuckoo/cuckooTests.h"

void usage(const char* argv0)
{
	std::cout << "Error! Please use:" << std::endl;
	std::cout << "\t 1. For unit test: " << argv0 << " -t" << std::endl;
	std::cout << "\t 2. For simulation (3 parties <=> 3 terminals): " << std::endl;;
	std::cout << "\t\t terminal: " << argv0 << " -p pIdx" << std::endl;

}
int main(int argc, char** argv)
{

	//myCuckooTest_stash();
	//Table_Based_Random_Test();
	//OPPRF2_EmptrySet_Test_Main();
	//OPPRFn_EmptrySet_Test_Main();
	//Transpose_Test();
	//OPPRF3_EmptrySet_Test_Main();
	//OPPRFnt_EmptrySet_Test_Main();
	//OPPRFnt_EmptrySet_Test_Main();
	//OPPRFn_Aug_EmptrySet_Test_Impl();
	//OPPRFnt_EmptrySet_Test_Impl();
	//OPPRF2_EmptrySet_Test_Main();
	//return 0;

	u64 trials = 1;
	std::vector<block> mSet;

	u64 pSetSize = 5, psiSecParam = 40, bitSize = 128;

	u64 nParties, tParties, opt_basedOPPRF, setSize, isAug;

	u64 roundOPPRF;


	switch (argc) {
	case 2: //unit test
		if (argv[1][0] == '-' && argv[1][1] == 'u')
			OPPRFnt_EmptrySet_Test_Main();
		break;

	case 7: //2PSI 
		if (argv[1][0] == '-' && argv[1][1] == 'n')
			nParties = atoi(argv[2]);
		else
		{
			usage(argv[0]);
			return 0;
		}

		if (argv[3][0] == '-' && argv[3][1] == 'm')
			setSize = 1 << atoi(argv[4]);
		else
		{
			usage(argv[0]);
			return 0;
		}

		if (argv[5][0] == '-' && argv[5][1] == 'p') {
			u64 pIdx = atoi(argv[6]);
			if (nParties == 2)
				party2(pIdx, setSize, trials);
			else
			{
				usage(argv[0]);
				return 0;
			}
		}
		else
		{
			usage(argv[0]);
			return 0;
		}
		break;
	case 9: //nPSI or optimized 3PSI
		cout << "9\n";
		if (argv[1][0] == '-' && argv[1][1] == 'n')
			nParties = atoi(argv[2]);
		else
		{
			usage(argv[0]);
			return 0;
		}
		if (argv[3][0] == '-' && argv[3][1] == 'r' && nParties == 3)
		{
			roundOPPRF = atoi(argv[4]);
			tParties = 2;
		}
		else if (argv[3][0] == '-' && argv[3][1] == 't')
			tParties = atoi(argv[4]);

		else if (argv[3][0] == '-' && argv[3][1] == 'a')
			opt_basedOPPRF = atoi(argv[4]);

		else
		{
			usage(argv[0]);
			return 0;
		}

		if (argv[5][0] == '-' && argv[5][1] == 'm')
			setSize = 1 << atoi(argv[6]);
		else
		{
			usage(argv[0]);
			return 0;
		}

		if (argv[7][0] == '-' && argv[7][1] == 'p') {
			u64 pIdx = atoi(argv[8]);
			if (roundOPPRF == 1 && nParties == 3)
			{
				//cout << nParties  << " " << roundOPPRF << " " << setSize << " " << pIdx << "\n";
				party3(pIdx, setSize, trials);

			}
			else if (argv[3][1] == 't')
			{
				//cout << nParties << " " << tParties << " " << setSize << " " << pIdx << "\n";
				tparty(pIdx, nParties, tParties, setSize, trials);
			}
			else if (argv[3][1] == 'a')
			{
				std::vector<std::vector<PRNG>> mPRNGSeeds(nParties);
				zero_sharing(mPRNGSeeds);
				//cout << nParties << " " << opt_basedOPPRF << " " << setSize << " " << pIdx << "\n";
				aug_party(pIdx, nParties, mSet.size(), mPRNGSeeds[pIdx], opt_basedOPPRF, trials);
			}
		}
		else
		{
			usage(argv[0]);
			return 0;
		}
		break;
	}

	return 0;
}
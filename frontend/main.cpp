
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
int miraclTestMain();

#include "cuckoo/cuckooTests.h"

int main(int argc, char** argv)
{
	//tt();
	std::cout << "myCuckooTest_stash" << std::endl;
	myCuckooTest_stash();

	std::cout << "myCuckooTest_bin" << std::endl;
	//myCuckooTest_bin();
    return 0;

    //LinearCode code;
    //code.loadBinFile(libOTe_DIR "/libPSI/Tools/bch511.bin");
    //std::vector<block> in(code.plaintextBlkSize()), out(code.codewordBlkSize());

    //Timer t;
    //t.setTimePoint("");
    //for (u64 j = 0; j < 1000000; ++j)
    //{
    //    code.encode(in, out);
    //}
    //t.setTimePoint("done");
    //std::cout << t << std::endl;

    //run_all();
    //return 0;
    //Ecc2mNumber_Test();
    //return 0;
    //miraclTestMain();
    //return 0;

    //test2();
    //return 0;
    //kpPSI();
    //return 0 ;
    //sim(); 
    //return 0;
    if (argc == 2)
    {
		//OPPRFSend();
		BarkOPRSend();


    }
    else if (argc == 3)
    {
		BarkOPRFRecv();
		//OPPRFRecv();
    }
    else
    {
#if 0
		Bit_Position_Map_Test();
		//Bit_Position_Test();
		//Bit_Position_Recursive_Test();
	//	 OPPRF_EmptrySet_Test_Impl1();
#else
        auto thrd = std::thread([]() {

			OPPRFRecv();
			//BarkOPRFRecv();
        });

		OPPRFSend();
		//BarkOPRSend();

        thrd.join();
        //otBin();

        //params();
        //bf(3);
        //KosTest();
        //run_all();
#endif
    }

    return 0;
}
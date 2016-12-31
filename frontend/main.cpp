
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

int main(int argc, char** argv)
{
	std::cout << "fffffffff\n";
	OPPRF3_EmptrySet_Test_Main();
	//OPPRFn_EmptrySet_Test_Main();
	
    if (argc == 2)
    {
		//OPPRFSend();
	//	BarkOPRSend();


    }
    else if (argc == 3)
    {
	//	BarkOPRFRecv();
		//OPPRFRecv();
    }
    else
    {
    }

    return 0;
}
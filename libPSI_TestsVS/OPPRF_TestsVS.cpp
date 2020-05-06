#include "stdafx.h"
//#include "CppUnitTest.h"
#include "OPPRF_Tests.h"
#include "EQ_Tests.h"
//#include "nPSI.h"
#include "Common.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WeGarbleTests
{
    TEST_CLASS(OPPRF_Tests)
    {
    public:

   //     TEST_METHOD(OPPRF_CuckooHasher_Test)
   //     {
   //         InitDebugPrinting();
			////OPPRF_CuckooHasher_Test_Impl();
   //     }

       /* TEST_METHOD(OPPRF_EmptrySet_Test)
        {
            InitDebugPrinting();
			OPPRF_EmptrySet_Test_Impl();
        }*/
		/*TEST_METHOD(nOPPRF_EmptrySet_Test)
		{
			InitDebugPrinting();
			nOPPRF_EmptrySet_Test_Impl();
		}*/

			TEST_METHOD(EQ_EmptrySet_Test)
		{
			InitDebugPrinting();
			EQ_EmptrySet_Test_Impl();

		}

		TEST_METHOD(OPPRFn_EmptrySet_Test)
		{
			InitDebugPrinting();
			OPPRFn_EmptrySet_Test_Impl();

		}

		TEST_METHOD(OPPRFn_Aug_EmptrySet_Test)
		{
			InitDebugPrinting();
			OPPRFn_Aug_EmptrySet_Test_Impl();
		}

   //     TEST_METHOD(OPPRF_FullSet_Test)
   //     {
   //         InitDebugPrinting();
			////OPPRF_FullSet_Test_Impl();
   //     }

   //     TEST_METHOD(OPPRF_SingltonSet_Test)
   //     {
   //         InitDebugPrinting();
			////OPPRF_SingltonSet_Test_Impl();
   //     }

 

       /* TEST_METHOD(Bit_Position_Test)
        {
            InitDebugPrinting();
			Bit_Position_Test_Impl();
        }*/


		/*TEST_METHOD(Bit_Position_Rec_Test)
		{
			InitDebugPrinting();
			Bit_Position_Recursive_Test_Impl();
		}*/
		/*TEST_METHOD(OPPRF_EmptrySet_hashing_Test)
		{
			InitDebugPrinting();
			OPPRF_EmptrySet_hashing_Test_Impl();
		}*/
		TEST_METHOD(OPPRF3_EmptrySet_Test)
		{
			InitDebugPrinting();
			OPPRF3_EmptrySet_Test_Impl();
		}

		/*TEST_METHOD(Hashing2Bins_Test)
		{
			InitDebugPrinting();
			hashing2Bins_Test_Impl();
		}*/

		TEST_METHOD(findScaleNumBins_Test)
		{
			InitDebugPrinting();
			findScaleNumBins_Test_Impl();
		}
		TEST_METHOD(findMaxBinSize_Test)
		{
			InitDebugPrinting();
			findMaxBinSize_Test_Impl();
		}
		TEST_METHOD(Bit_Position_Random_Test)
		{
			InitDebugPrinting();
			Bit_Position_Random_Test_Impl();
		}
		TEST_METHOD(testShareValue_Test)
		{
			InitDebugPrinting();
			testShareValue();
		}

		TEST_METHOD(OPPRFnt_EmptrySet_Test)
		{
			InitDebugPrinting();
			OPPRFnt_EmptrySet_Test_Impl();
		}

		TEST_METHOD(polynomial_Test)
		{
			InitDebugPrinting();
			polynomial_Test_Impl();
		}

		TEST_METHOD(GBF_Test)
		{
			InitDebugPrinting();
			GBF_Test_Impl();
		}
		TEST_METHOD(OPPRF2_EmptrySet_Test)
		{
			InitDebugPrinting();
			OPPRF2_EmptrySet_Test_Impl();
		}
		
		/*TEST_METHOD(Channel_Test)
		{
			InitDebugPrinting();
			Channel_Test_Impl();
		}*/


    };
}
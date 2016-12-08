#include "stdafx.h"
#include "CppUnitTest.h"
#include "OPPRF_Tests.h"
#include "Common.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WeGarbleTests
{
    TEST_CLASS(OPPRF_Tests)
    {
    public:

        TEST_METHOD(OPPRF_CuckooHasher_Test)
        {
            InitDebugPrinting();
			//OPPRF_CuckooHasher_Test_Impl();
        }

        TEST_METHOD(OPPRF_EmptrySet_Test)
        {
            InitDebugPrinting();
			OPPRF_EmptrySet_Test_Impl();
        }

        TEST_METHOD(OPPRF_FullSet_Test)
        {
            InitDebugPrinting();
			//OPPRF_FullSet_Test_Impl();
        }

        TEST_METHOD(OPPRF_SingltonSet_Test)
        {
            InitDebugPrinting();
			//OPPRF_SingltonSet_Test_Impl();
        }

 

        TEST_METHOD(Bit_Position_Test)
        {
            InitDebugPrinting();
			Bit_Position_Test_Impl();
        }
    };
}
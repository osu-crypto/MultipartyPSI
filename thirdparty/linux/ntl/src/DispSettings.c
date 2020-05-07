
#include <NTL/ctools.h>

#include <iostream>
using namespace std;


#define make_string_aux(x) #x
#define make_string(x) make_string_aux(x)


int main()
{

   cout << "\n\n";
   cout << "/***************************\n";
   cout << "Basic Configuration Options:\n";


#ifdef NTL_LEGACY_NO_NAMESPACE
   cout << "NTL_LEGACY_NO_NAMESPACE\n";
#endif


#ifdef NTL_LEGACY_INPUT_ERROR
   cout << "NTL_LEGACY_INPUT_ERROR\n";
#endif


#ifdef NTL_THREADS
   cout << "NTL_THREADS\n";
#endif

#ifdef NTL_DISABLE_TLS_HACK
   cout << "NTL_DISABLE_TLS_HACK\n";
#endif

#ifdef NTL_ENABLE_TLS_HACK
   cout << "NTL_ENABLE_TLS_HACK\n";
#endif

#ifdef NTL_EXCEPTIONS
   cout << "NTL_EXCEPTIONS\n";
#endif

#ifdef NTL_THREAD_BOOST
   cout << "NTL_THREAD_BOOST\n";
#endif


#ifdef NTL_GMP_LIP
   cout << "NTL_GMP_LIP\n";
#endif


#ifdef NTL_GF2X_LIB
   cout << "NTL_GF2X_LIB\n";
#endif

#ifdef NTL_LONG_LONG_TYPE
   cout << "NTL_LONG_LONG_TYPE: ";
   cout << make_string(NTL_LONG_LONG_TYPE) << "\n";
#endif

#ifdef NTL_UNSIGNED_LONG_LONG_TYPE
   cout << "NTL_UNSIGNED_LONG_LONG_TYPE: ";
   cout << make_string(NTL_UNSIGNED_LONG_LONG_TYPE) << "\n";
#endif


#ifdef NTL_X86_FIX
   cout << "NTL_X86_FIX\n";
#endif

#ifdef NTL_NO_X86_FIX
   cout << "NTL_NO_X86_FIX\n";
#endif

#ifdef NTL_NO_INIT_TRANS
   cout << "NTL_NO_INIT_TRANS\n";
#endif

#ifdef NTL_CLEAN_INT
   cout << "NTL_CLEAN_INT\n";
#endif

#ifdef NTL_CLEAN_PTR
   cout << "NTL_CLEAN_PTR\n";
#endif

#ifdef NTL_RANGE_CHECK
   cout << "NTL_RANGE_CHECK\n";
#endif


#ifdef NTL_LEGACY_SP_MULMOD
   cout << "NTL_LEGACY_SP_MULMOD\n";
#endif


#ifdef NTL_DISABLE_LONGDOUBLE
   cout << "NTL_DISABLE_LONGDOUBLE\n";
#endif

#ifdef NTL_DISABLE_LONGLONG
   cout << "NTL_DISABLE_LONGLONG\n";
#endif

#ifdef NTL_DISABLE_LL_ASM
   cout << "NTL_DISABLE_LL_ASM\n";
#endif

#ifdef NTL_MAXIMIZE_SP_NBITS
   cout << "NTL_MAXIMIZE_SP_NBITS\n";
#endif


cout << "\n";
cout << "Resolution of double-word types:\n";
cout << make_string(NTL_LL_TYPE) << "\n";
cout << make_string(NTL_ULL_TYPE) << "\n";


cout << "\n";
cout << "Performance Options:\n";

#ifdef NTL_LONG_LONG
   cout << "NTL_LONG_LONG\n";
#endif

#ifdef NTL_AVOID_FLOAT
   cout << "NTL_AVOID_FLOAT\n";
#endif


#ifdef NTL_SPMM_ULL
   cout << "NTL_SPMM_ULL\n";
#endif


#ifdef NTL_SPMM_ASM
   cout << "NTL_SPMM_ASM\n";
#endif


#ifdef NTL_AVOID_BRANCHING
   cout << "NTL_AVOID_BRANCHING\n";
#endif

#ifdef NTL_FFT_BIGTAB
   cout << "NTL_FFT_BIGTAB\n";
#endif

#ifdef NTL_FFT_LAZYMUL
   cout << "NTL_FFT_LAZYMUL\n";
#endif


#ifdef NTL_TBL_REM
   cout << "NTL_TBL_REM\n";
#endif

#ifdef NTL_TBL_REM_LL
   cout << "NTL_TBL_REM_LL\n";
#endif

#ifdef NTL_CRT_ALTCODE
   cout << "NTL_CRT_ALTCODE\n";
#endif

#ifdef NTL_CRT_ALTCODE_SMALL
   cout << "NTL_CRT_ALTCODE_SMALL\n";
#endif


#ifdef NTL_GF2X_ALTCODE
   cout << "NTL_GF2X_ALTCODE\n";
#endif


#ifdef NTL_GF2X_ALTCODE1
   cout << "NTL_GF2X_ALTCODE1\n";
#endif


#ifdef NTL_GF2X_NOINLINE
   cout << "NTL_GF2X_NOINLINE\n";
#endif

#ifdef NTL_PCLMUL
   cout << "NTL_PCLMUL\n";
#endif


   cout << "***************************/\n";
   cout << "\n\n";

   return 0;
}

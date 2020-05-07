
#ifndef NTL_FFT__H
#define NTL_FFT__H

#include <NTL/ZZ.h>
#include <NTL/vector.h>
#include <NTL/vec_long.h>
#include <NTL/SmartPtr.h>
#include <NTL/LazyTable.h>

NTL_OPEN_NNS

#define NTL_FFTFudge (4)
// This constant is used in selecting the correct
// number of FFT primes for polynomial multiplication
// in ZZ_pX and zz_pX.  Set at 4, this allows for
// two FFT reps to be added or subtracted once,
// before performing CRT, and leaves a reasonable margin for error.
// Don't change this!

#define NTL_FFTMaxRootBnd (NTL_SP_NBITS-2)
// Absolute maximum root bound for FFT primes.
// Don't change this!

#if (25 <= NTL_FFTMaxRootBnd)
#define NTL_FFTMaxRoot (25)
#else
#define NTL_FFTMaxRoot  NTL_FFTMaxRootBnd
#endif
// Root bound for FFT primes.  Held to a maximum
// of 25 to avoid large tables and excess precomputation,
// and to keep the number of FFT primes needed small.
// This means we can multiply polynomials of degree less than 2^24.  
// This can be increased, with a slight performance penalty.




class FFTVectorPair {
public:
   Vec<long> wtab_precomp;
   Vec<mulmod_precon_t> wqinvtab_precomp;
};

typedef LazyTable<FFTVectorPair, NTL_FFTMaxRoot+1> FFTMultipliers;


class FFTMulTabs {
public:

   FFTMultipliers MulTab[2];

};

class zz_pInfoT; // forward reference, defined in lzz_p.h


struct FFTPrimeInfo {
   long q;   // the prime itself
   mulmod_t qinv;   // 1/((wide_double) q) -- but subject to change!!
   double qrecip;   // 1/double(q)

   SmartPtr<zz_pInfoT> zz_p_context; 
   // pointer to corresponding zz_p context, which points back to this 
   // object in the case of a non-user FFT prime

   Vec<long> RootTable[2];
   //   RootTable[0][j] = w^{2^{MaxRoot-j}},
   //                     where w is a primitive 2^MaxRoot root of unity
   //                     for q
   // RootInvTable[1][j] = 1/RootTable[0][j] mod q


   Vec<long> TwoInvTable;
   // TwoInvTable[j] = 1/2^j mod q

   Vec<mulmod_precon_t> TwoInvPreconTable;
   // mulmod preconditioning data

   UniquePtr< FFTMulTabs > bigtab;

};

void InitFFTPrimeInfo(FFTPrimeInfo& info, long q, long w, bool bigtab);


#define NTL_MAX_FFTPRIMES (20000)
// for a thread-safe implementation, it is most convenient to
// impose a reasonabel upper bound on he number of FFT primes.
// without this restriction, a growing table would have to be
// relocated in one thread, leaving dangling pointers in 
// another thread.  Each entry in the table is just a poiner,
// so this does not incur too much space overhead.
// One could alo implement a 2D-table, which would allocate
// rows on demand, thus reducing wasted space at the price
// of extra arithmetic to actually index into the table.
// This may be an option to consider at some point.

// At the current setting of 20000, on 64-bit machines with 50-bit
// FFT primes, this allows for polynomials with 20*50/2 = 500K-bit 
// coefficients, while the table itself takes 160KB.


typedef LazyTable<FFTPrimeInfo, NTL_MAX_FFTPRIMES> FFTTablesType;

extern FFTTablesType FFTTables;
// a truly GLOBAL variable, shared among all threads


static inline 
long GetFFTPrime(long i)
{
   return FFTTables[i]->q;
}

static inline 
mulmod_t GetFFTPrimeInv(long i)
{
   return FFTTables[i]->qinv;
}

static inline 
double GetFFTPrimeRecip(long i)
{
   return FFTTables[i]->qrecip;
}



long CalcMaxRoot(long p);
// calculates max power of two supported by this FFT prime.

void UseFFTPrime(long index);
// allocates and initializes information for FFT prime


void FFT(long* A, const long* a, long k, const FFTPrimeInfo& info, long dir);
// the low-level FFT routine.
// computes a 2^k point FFT modulo q = info.q
// dir == 0 => forward direction (using roots)
// dir == 1 => backwards direction (using inverse roots)





static inline
void FFTFwd(long* A, const long *a, long k, const FFTPrimeInfo& info)
// Slightly higher level interface...using the ith FFT prime
{
   FFT(A, a, k, info, 0);
}


static inline
void FFTFwd(long* A, const long *a, long k, long i)
{
   FFTFwd(A, a, k, *FFTTables[i]);
}

static inline
void FFTRev(long* A, const long *a, long k, const FFTPrimeInfo& info)
// Slightly higher level interface...using the ith FFT prime
{
   FFT(A, a, k, info, 1);
}

static inline
void FFTRev(long* A, const long *a, long k, long i)
{
   FFTRev(A, a, k, *FFTTables[i]);
}

static inline
void FFTMulTwoInv(long* A, const long *a, long k, const FFTPrimeInfo& info)
{
   VectorMulModPrecon(1L << k, A, a, info.TwoInvTable[k], info.q, 
                      info.TwoInvPreconTable[k]);
}

static inline
void FFTMulTwoInv(long* A, const long *a, long k, long i)
{
   FFTMulTwoInv(A, a, k, *FFTTables[i]);
}

static inline 
void FFTRev1(long* A, const long *a, long k, const FFTPrimeInfo& info)
// FFTRev + FFTMulTwoInv
{
   FFTRev(A, a, k, info);
   FFTMulTwoInv(A, A, k, info);
}

static inline 
void FFTRev1(long* A, const long *a, long k, long i)
{
   FFTRev1(A, a, k, *FFTTables[i]);
}


long IsFFTPrime(long n, long& w);
// tests if n is an "FFT prime" and returns corresponding root




NTL_CLOSE_NNS

#endif

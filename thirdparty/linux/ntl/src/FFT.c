
#include <NTL/FFT.h>
#include <NTL/new.h>


/********************************************************************

This is an implementation of a "small prime" FFT, which lies at the heart of
the ZZ_pX arithmetic, as well as some other applications.

The basic algorithm is loosely based on the routine in the Cormen, Leiserson,
Rivest, and Stein book on algorithms.


CACHE PERFORMANCE

Some attention has been paid to cache performance, but there is still more that
could be done.  


The bit-reverse-copy (BRC) algorithm is a simple table-driven algorithm up to
a certain theshold, and then switches to the COBRA algorithm from  Carter and
Gatlin, "Towards an optimal bit-reversal permutation algorithm", FOCS 1998.
I've found that COBRA helps, but not much: just 5-10%.  I've also found that
getting rid of BRC altogether leads to another 5-10% improvement.  These
numbers are based on experiments with 2^{17}- and 2^{19}-point FFTs, looping
over 50 different primes on a Core 2 Duo machine.

One could get rid of bit-reverse-copy altogether.  The current FFT routines all
implement what is called Decimation-In-Time (DIT), which means that inputs are
bit reversed.  One can also implement the FFT using Decimation-In-Frequency
(DIF), which means that the outputs are bit reversed.  One can get rid of the
bit reversals for doing convolutions by simply doing the forward FFT using
DIF-FFT and and the reverse FFT using DIT-FFT.  This would allow one to simply
eliminate all of the bit-reversal steps, which would lead to some nontrivial
savings.  However, there are a few places in NTL where I rely on the ordering
of elements within an FFTRep to be their "natural ordering".  The reduce and
AddExpand routines in ZZ_pX come to mind (which actually may become simpler),
along with like RevToFFTRep and RevFromFFTRep (which may be trickier).  Anyway,
because BRC doesn't seem to be a big problem right now, it doesn't seem worth
worrying about this.


Within the FFT algorithm itself, I have not tried anything like Bailey's 4-step
algorithm.  Maybe this should be tested.  However, I somehow doubt that
anything more than modest gains will be achieved, since most modern processors
now employ a kind of memory prefetching technique, to keep the cache filled
with memory locations that are likely to be used next.  Moreover, the FFT
algorithm used here accesses memory for the most part in small, sequential
strides, which meshes well with hardware prefetching.  The paper "Algorithms to
Take Advantage of Hardware Prefetching" [Pan, Cherng, Dick, Ladner, Workshop on
Algorithm Engineering and Experiments, 2007] contains some interesting
experiments and useful background information.  Anyway, there is still room for
more experimentation.



SINGLE-PRECISION MODULAR ARITHMETIC

The implementation of arithmetic modulo n, where n is a "word sized" integer is
critical to the performance of the FFT.  Such word-sized modular arithmetic is
used throughout many other parts of NTL, and is a part of the external,
documented interface.

As NTL was initially built on top of Arjen Lenstra's LIP software, I stole a
lot of ideas from LIP.  One very nice ideas was LIP's way of handling
single-precision modular arithmetic.  Back in those days (the early 1990's), I
was targeting 32-machines, mainly SPARC stations.  LIP's stratgey was to
restrict n to 30 bits, and to compute a*b % n, where 0 <= a, b < n, the
follwong was computed:

   long q = long(double(a) * double(b) / double(n));
   long r = a*b - q*n;
   if (r >= n) 
      r -= n;
   else if (r < 0)
      r += n;

With quite reasonable assumptions about floating point (certainly, anything
even remotely close to IEEE 64-bit doubles), the computation of q always gives
the true quotient floor(a*b / n), plus or minus 1.  The computation of r is
done modulo the 2^{word size}, and the following if/then/else adjusts r as
necessary.  To be more portable, some of these computations should really be
done using unsigned arithmetic, but that is not so important here.  Also, the
adjustment steps can be replaced by simple non-branching instrictions sequences
involving SHIFT, AND, and ADD/SUB instructions.  On many modern machines, this
is usually faster and NTL uses this non-branching strategy.

Other simple optimizations can be done, such as precomputing 1/double(n) when n
remains fixed for many computations, as is often the case.  

Note also that this strategy works perfectly well even when a or b are larger
than n, but the quotient itself is bounded by 2^30.

This strategy worked well for many years.  I had considered employing
"Montgomery multiplication", but did not do so for a couple of reasons:
  1) it would require non-portable code, because Montgomery multiplication
     requires the computation of two-word products,
  2) I did not like the idea of working with "alternative representations"
     for integers mod n, as this would make the interfaces more awkward.

At some point in the early 2000's, this strategy was starting to slow things
down, as floating point arithmetic, especially the integer/floating point
conversions, was starting to slow down relative to integer arithmetic.  This
was especially true on x86 machines, which by this time was starting to become
the most important target.  As it happens, later in the 2000's, as the x86
platforms started to use SSE instructions in lieu of the old x87 FPU
instructions, this speed differential again became less of a problem.
Nevertheless, I introduced some new techniques that speed things up across a
variety of platforms.  I introduced this new technique in NTL 5.4 back in 2005.
I never claimed it was particularly new, and I never really documented many
details about it, but since then, it has come to be known as "Shoup
multiplcation" in a few papers, so I'll accept that. :-)  The paper "Faster
arithmetic for number-theoretic transforms" [David Harvey, J. Symb. Comp. 60
(2014) 113–119] seems to be the first place where it is discussed in detail,
and Harvey's paper also contains some improvements which I discuss below.

The basic idea is that in many computations, not only n, but one of the
arguments, say b, remains fixed for many computatations of a*b % n, and so we
can afford to do a little precomputation, based on b and n, to speed things up.
This approach does require the ability to compute double-word products
(actually, just the high word of the product), but it still presents the same
basic interface as before (i.e., no awkward, alternative representations);
moreover, on platforms where we can't get double-word products, the
implementation falls back to the old floating point strategy, and client code
need not be aware of this.

The basic idea is this: suppose 0 <= n < 2^w, and 0 <= a < 2^w, and 0 <= b < n.
We precompute bninv = floor(2^w*b/n).  Then if we compute q =
floor(a*bninv/2^w), it can be argued that q is either floor(a*b/n), or is 1 too
small.  The computation of bninv can be done using the floating point
techniques described above.  The computation of q can be done by computing the
high word of a double-word product (it helps if bninv is left-shifted an
appropriate amount first).  Once we have q, we can compute a*b - q*n as before,
and adjust (but now only one adjustment is needed).  So after the
precomputation.  the whole operation takes 3 multiplies (one doube-word and two
single-word), and a small handful of simple instructions (adds, shifts, etc).
Moreover, two of the three multiplies can start in parallel, on platforms where
this is possible.

David Harvey noticed that because on modern machines, multiplies are really not
that slow compared to additions, the cost of all of the adjustments (in the
MulMod, as well as in the AddMod and SubMod's in the basic FFT butterfly steps)
starts to dominate the cost of the FFT. Indeed, with a straightforward
implementation of the above ideas, there are three multiplies and three
adjustment steps in each butterfly step.  David's idea was to work with
redundant representations mod n, in the range [0..4*n), and thus reduce the
number of adjustments per butterfly from three to one.  I've implemented this
idea here, and it does indeed make a significant difference, which is even more
pronounced when all of the FFT multipliers b and corresponding bninv values are
precomputed.  My initial implementation of David's ideas (v6.0 in 2013) only
implemented his approach with these precomputated tables: it seemed that
without these tables, it was not a significant improvement.  However, I later
figured out how to reduce the cost of computing all the necessary data "on the
fly", in a way that seems only slightly (10-15%) slower overall.  I introduced
this in v9.1 in 2015, and set things up so that now the pre-computed tables are
still used, but not exclusively, in such a way as to reduce the memory used by
these tables for very large polynomials (either very high degree or lots of FFT
primes).   The idea here is simple, but I haven't seen it discussed elsewhere,
so I'll document the basic idea here.

Suppose we have the preconditioners for a and b, and want a*b % n along with
the preconditioner for a*b % n.

For a, let us suppose that we have both q1 and r1, where:
   2^w*a = n*q1 + r1
We can obtain both q1 and r1 using floating point techniques.

Step 1. Compute a*b % n, using the integer-only MulMod, using
either the preconditioner for either a or b.

Step 2. Compute q2 and r2 such that
   r1*b = n*q2 + r2
We can obtain these using the integer-only MulMod, preconditioned on b.
Actually, we only need q2, not r2.

Step 3. Compute
   q3 = q1*b + q2 mod 2^w
which we can compute with just a single-word multiply and an addition.

One can easily show that the value q3 computed above is indeed the
preconditioner for a*b % n.  

Note that, in theory, if the computation in Step 2 is done using the
preconditioner for a (i.e., q1), then the multiplication q1*b in Step 3 should
not really be necessary (assuming that computing both high and low words of a
doube-wprd product is no more expensive than just computing the low word).
However, none of the compilers I've used have been able to perform that
optimization.


64-BIT MACHINES

Prior to v9.0 of NTL, on 64 bits, the modulus n was restricted to 50 bits, in
order to allow the use of double-precision techniques, as double's have 53 bits
of precisions.  However, since the x86-64 is such an importnat target, and the
one can still access the old x87 FPU, which provided 64-bit precision, the
bound on n on such platforms is now 60 bits.  Actually, 62 bits could be
supported, but other things (namely, the TBL_REM implementation in
g_lip_impl.h) start to slow down if 62 bits are used, so 60 seems like a good
compromose.  Currently,  60-bit moduli are available only when using gcc on
x86-64 machines, and when compiling NTL with GMP. 

Now, the FPU-based multiplies are in fact a bit slower than the SSE-based
multiplies. However, with the preconditioned all-integer MulMod's now used
extensively on almost all critical paths within NTL, this does not really
matter, and in fact, many things get faster with the wider moduli, so overall,
it is a net performance gain.


FUTURE TRENDS

In the future, I might also experiment with other MulMod techniques, such as
those described in "Improved Division by Invariant Integers" [Moeller,
Granlund, IEEE Transactions on Computers, June 2010].  This might allow for,
say, 60-bit moduli on 64-bit machines that don't have extended double
precision.   It is not clear how the performance of this would compare with the
floating-point methods; however, it probably doesn't matter too much, as the
preconditioned MulMod's are the most important ones.

It might also be useful to go back and reconsider Montgomery multiplication, at
least for "internal" use, like the FFT.  However, I doubt that this will help
significantly.

As mentioned above, it could be useful to experiment with more cache-friendly
variants of the FFT, like Bailey's 4-step method.  I could also experiment with
using the DIF/DIT.  This affects some code outside of FFT as well (in ZZ_pX and
zz_pX, like reduce and AddeExpand), but should not affect any documented
interfaces.

Another direction to consider is exploiting concurrency.  Besides using
multiple cores to parallelize things at a higher level, it would be nice to
exploit newer SIMD instructions.  Unfortunately, as of now (early 2015), these
don't seem to have the functionality I need.  A 64-bit x 64-bit -> low order
64-bit instruction is supposed to be available soon in the new AVX-512
instruction set.  That would be a good start, but I would really like to get
the high-order 64-bits too. Maybe that will come someday.  In the mean time, it
might be fun to experiment with using the AVX-512 instructions that will be
available, which will at least allow at least a floating-point-based
implementation, or an all-integer implementation with emulated MulHi.  I have
no idea how performance will compare.



********************************************************************/







//  #define NTL_BRC_TEST
//  Flag to test the cost of "bit reverse copy"


#define NTL_FFT_BIGTAB_LIMIT (200)
#ifndef NTL_BRC_TEST
#define NTL_FFT_BIGTAB_MAXROOT (17)
#else
#define NTL_FFT_BIGTAB_MAXROOT (25)
#endif
// big tables are only used for the first NTL_FFT_BIGTAB_LIMIT primes,
// and then only for k-values at most NTL_FFT_BIGTAB_MAXROOT

// NOTE: in newer versions of NTL (v9.1 and later), the BIGTAB
// code is only about 5-15% faster than the non-BIGTAB code, so
// this is not a great time/space trade-off.

// NOTE: NTL_FFT_BIGTAB_MAXROOT is set independently of the parameter
// NTL_FFTMaxRoot defined in FFT.h (and which is typically 25).
// The space for the LazyTable FFTMultipliers could be reduced a bit
// by using min(NTL_FFT_BIGTAB_MAXROOT, NTL_FFTMaxRoot) + 1 for the
// size of these tables.



NTL_START_IMPL


FFTTablesType FFTTables;
// a truly GLOBAL variable, shared among all threads



long IsFFTPrime(long n, long& w)
{
   long  m, x, y, z;
   long j, k;


   if (n <= 1 || n >= NTL_SP_BOUND) return 0;

   if (n % 2 == 0) return 0;

   if (n % 3 == 0) return 0;

   if (n % 5 == 0) return 0;

   if (n % 7 == 0) return 0;
   
   m = n - 1;
   k = 0;
   while ((m & 1) == 0) {
      m = m >> 1;
      k++;
   }

   for (;;) {
      x = RandomBnd(n);

      if (x == 0) continue;
      z = PowerMod(x, m, n);
      if (z == 1) continue;

      x = z;
      j = 0;
      do {
         y = z;
         z = MulMod(y, y, n);
         j++;
      } while (j != k && z != 1);

      if (z != 1 || y !=  n-1) return 0;

      if (j == k) 
         break;
   }

   /* x^{2^k} = 1 mod n, x^{2^{k-1}} = -1 mod n */

   long TrialBound;

   TrialBound = m >> k;
   if (TrialBound > 0) {
      if (!ProbPrime(n, 5)) return 0;
   
      /* we have to do trial division by special numbers */
   
      TrialBound = SqrRoot(TrialBound);
   
      long a, b;
   
      for (a = 1; a <= TrialBound; a++) {
         b = (a << k) + 1;
         if (n % b == 0) return 0; 
      }
   }

   /* n is an FFT prime */


   for (j = NTL_FFTMaxRoot; j < k; j++) {
      x = MulMod(x, x, n);
   }

   w = x;

   return 1;
}


static
void NextFFTPrime(long& q, long& w, long index)
{
   static long m = NTL_FFTMaxRootBnd + 1;
   static long k = 0;
   // m and k are truly GLOBAL variables, shared among
   // all threads.  Access is protected by a critical section
   // guarding FFTTables

   static long last_index = -1;
   static long last_m = 0;
   static long last_k = 0;

   if (index == last_index) {
      // roll back m and k...part of a simple error recovery
      // strategy if an exception was thrown in the last 
      // invocation of UseFFTPrime...probably of academic 
      // interest only

      m = last_m;
      k = last_k;
   }
   else {
      last_index = index;
      last_m = m;
      last_k = k;
   }

   long t, cand;

   for (;;) {
      if (k == 0) {
         m--;
         if (m < 5) ResourceError("ran out of FFT primes");
         k = 1L << (NTL_SP_NBITS-m-2);
      }

      k--;

      cand = (1L << (NTL_SP_NBITS-1)) + (k << (m+1)) + (1L << m) + 1;

      if (!IsFFTPrime(cand, t)) continue;
      q = cand;
      w = t;
      return;
   }
}


long CalcMaxRoot(long p)
{
   p = p-1;
   long k = 0;
   while ((p & 1) == 0) {
      p = p >> 1;
      k++;
   }

   if (k > NTL_FFTMaxRoot)
      return NTL_FFTMaxRoot;
   else
      return k; 
}


void InitFFTPrimeInfo(FFTPrimeInfo& info, long q, long w, bool bigtab)
{
   mulmod_t qinv = PrepMulMod(q);

   long mr = CalcMaxRoot(q);

   info.q = q;
   info.qinv = qinv;
   info.qrecip = 1/double(q);
   info.zz_p_context = 0;


   info.RootTable[0].SetLength(mr+1);
   info.RootTable[1].SetLength(mr+1);
   info.TwoInvTable.SetLength(mr+1);
   info.TwoInvPreconTable.SetLength(mr+1);

   long *rt = &info.RootTable[0][0];
   long *rit = &info.RootTable[1][0];
   long *tit = &info.TwoInvTable[0];
   mulmod_precon_t *tipt = &info.TwoInvPreconTable[0];

   long j;
   long t;

   rt[mr] = w;
   for (j = mr-1; j >= 0; j--)
      rt[j] = MulMod(rt[j+1], rt[j+1], q);

   rit[mr] = InvMod(w, q);
   for (j = mr-1; j >= 0; j--)
      rit[j] = MulMod(rit[j+1], rit[j+1], q);

   t = InvMod(2, q);
   tit[0] = 1;
   for (j = 1; j <= mr; j++)
      tit[j] = MulMod(tit[j-1], t, q);

   for (j = 0; j <= mr; j++)
      tipt[j] = PrepMulModPrecon(tit[j], q, qinv);

   if (bigtab)
      info.bigtab.make();
}


#ifndef NTL_WIZARD_HACK
SmartPtr<zz_pInfoT> Build_zz_pInfo(FFTPrimeInfo *info);
#else
SmartPtr<zz_pInfoT> Build_zz_pInfo(FFTPrimeInfo *info) { return 0; }
#endif

void UseFFTPrime(long index)
{
   if (index < 0) LogicError("invalud FFT prime index");
   if (index >= NTL_MAX_FFTPRIMES) ResourceError("FFT prime index too large");

   do {  // NOTE: thread safe lazy init
      FFTTablesType::Builder bld(FFTTables, index+1);
      long amt = bld.amt();
      if (!amt) break;

      long first = index+1-amt;
      // initialize entries first..index

      long i;
      for (i = first; i <= index; i++) {
         UniquePtr<FFTPrimeInfo> info;
         info.make();

         long q, w;
         NextFFTPrime(q, w, i);

         bool bigtab = false;

#ifdef NTL_FFT_BIGTAB
         if (i < NTL_FFT_BIGTAB_LIMIT)
            bigtab = true;
#endif

         InitFFTPrimeInfo(*info, q, w, bigtab);
         info->zz_p_context = Build_zz_pInfo(info.get());
         bld.move(info);
      }

   } while (0);
}





#define NTL_PIPELINE
//  Define to gets some software pipelining...actually seems
//  to help somewhat

#define NTL_LOOP_UNROLL
//  Define to unroll some loops.  Seems to help a little

// FIXME: maybe the above two should be tested by the wizard


static
long RevInc(long a, long k)
{
   long j, m;

   j = k; 
   m = 1L << (k-1);

   while (j && (m & a)) {
      a ^= m;
      m >>= 1;
      j--;
   }
   if (j) a ^= m;
   return a;
}


// FIXME: This could potentially be shared across threads, using
// a "lazy table".
static inline 
Vec<long> *get_brc_mem()
{
   NTL_TLS_LOCAL_INIT(Vec< Vec<long> >, brc_mem_vec, (INIT_SIZE, NTL_FFTMaxRoot+1));
   return brc_mem_vec.elts();
}



#if 0


static
void BitReverseCopy(long * NTL_RESTRICT A, const long * NTL_RESTRICT a, long k)
{
   Vec<long> *brc_mem = get_brc_mem();

   long n = 1L << k;
   long* NTL_RESTRICT rev;
   long i, j;

   rev = brc_mem[k].elts();
   if (!rev) {
      brc_mem[k].SetLength(n);
      rev = brc_mem[k].elts();
      for (i = 0, j = 0; i < n; i++, j = RevInc(j, k))
         rev[i] = j;
   }

   for (i = 0; i < n; i++)
      A[rev[i]] = a[i];
}


static
void BitReverseCopy(unsigned long * NTL_RESTRICT A, const long * NTL_RESTRICT a, long k)
{
   Vec<long> *brc_mem = get_brc_mem();

   long n = 1L << k;
   long* NTL_RESTRICT rev;
   long i, j;

   rev = brc_mem[k].elts();
   if (!rev) {
      brc_mem[k].SetLength(n);
      rev = brc_mem[k].elts();
      for (i = 0, j = 0; i < n; i++, j = RevInc(j, k))
         rev[i] = j;
   }

   for (i = 0; i < n; i++)
      A[rev[i]] = a[i];
}

#else



#define NTL_BRC_THRESH (12)
#define NTL_BRC_Q (5)

// Must have NTL_BRC_THRESH > 2*NTL_BRC_Q
// Should also have (1L << (2*NTL_BRC_Q)) small enough
// so that we can fit that many long's into the cache


static
long *BRC_init(long k)
{
   Vec<long> *brc_mem = get_brc_mem();

   long n = (1L << k);
   brc_mem[k].SetLength(n);
   long *rev = brc_mem[k].elts();
   long i, j;
   for (i = 0, j = 0; i < n; i++, j = RevInc(j, k))
      rev[i] = j;
   return rev;
}


static
void BasicBitReverseCopy(long * NTL_RESTRICT B, 
                         const long * NTL_RESTRICT A, long k)
{
   Vec<long> *brc_mem = get_brc_mem();

   long n = 1L << k;
   long* NTL_RESTRICT rev;
   long i, j;

   rev = brc_mem[k].elts();
   if (!rev) rev = BRC_init(k);

   for (i = 0; i < n; i++)
      B[rev[i]] = A[i];
}



static
void COBRA(long * NTL_RESTRICT B, const long * NTL_RESTRICT A, long k)
{
   Vec<long> *brc_mem = get_brc_mem();

   NTL_TLS_LOCAL(Vec<long>, BRC_temp);

   long q = NTL_BRC_Q;
   long k1 = k - 2*q;
   long * NTL_RESTRICT rev_k1, * NTL_RESTRICT rev_q;
   long *NTL_RESTRICT T;
   long a, b, c, a1, b1, c1;
   long i, j;

   rev_k1 = brc_mem[k1].elts();
   if (!rev_k1) rev_k1 = BRC_init(k1);

   rev_q = brc_mem[q].elts();
   if (!rev_q) rev_q = BRC_init(q);

   T = BRC_temp.elts();
   if (!T) {
      BRC_temp.SetLength(1L << (2*q));
      T = BRC_temp.elts();
   }

   for (b = 0; b < (1L << k1); b++) {
      b1 = rev_k1[b]; 
      for (a = 0; a < (1L << q); a++) {
         a1 = rev_q[a]; 
         for (c = 0; c < (1L << q); c++) 
            T[(a1 << q) + c] = A[(a << (k1+q)) + (b << q) + c]; 
      }

      for (c = 0; c < (1L << q); c++) {
         c1 = rev_q[c];
         for (a1 = 0; a1 < (1L << q); a1++) 
            B[(c1 << (k1+q)) + (b1 << q) + a1] = T[(a1 << q) + c];
      }
   }
}

static
void BitReverseCopy(long * NTL_RESTRICT B, const long * NTL_RESTRICT A, long k)
{
   if (k <= NTL_BRC_THRESH) 
      BasicBitReverseCopy(B, A, k);
   else
      COBRA(B, A, k);
}


static
void BasicBitReverseCopy(unsigned long * NTL_RESTRICT B, 
                         const long * NTL_RESTRICT A, long k)
{
   Vec<long> *brc_mem = get_brc_mem();

   long n = 1L << k;
   long* NTL_RESTRICT rev;
   long i, j;

   rev = brc_mem[k].elts();
   if (!rev) rev = BRC_init(k);

   for (i = 0; i < n; i++)
      B[rev[i]] = A[i];
}



static
void COBRA(unsigned long * NTL_RESTRICT B, const long * NTL_RESTRICT A, long k)
{
   Vec<long> *brc_mem = get_brc_mem();

   NTL_TLS_LOCAL(Vec<unsigned long>, BRC_temp);

   long q = NTL_BRC_Q;
   long k1 = k - 2*q;
   long * NTL_RESTRICT rev_k1, * NTL_RESTRICT rev_q;
   unsigned long *NTL_RESTRICT T;
   long a, b, c, a1, b1, c1;
   long i, j;

   rev_k1 = brc_mem[k1].elts();
   if (!rev_k1) rev_k1 = BRC_init(k1);

   rev_q = brc_mem[q].elts();
   if (!rev_q) rev_q = BRC_init(q);

   T = BRC_temp.elts();
   if (!T) {
      BRC_temp.SetLength(1L << (2*q));
      T = BRC_temp.elts();
   }

   for (b = 0; b < (1L << k1); b++) {
      b1 = rev_k1[b]; 
      for (a = 0; a < (1L << q); a++) {
         a1 = rev_q[a]; 
         for (c = 0; c < (1L << q); c++) 
            T[(a1 << q) + c] = A[(a << (k1+q)) + (b << q) + c]; 
      }

      for (c = 0; c < (1L << q); c++) {
         c1 = rev_q[c];
         for (a1 = 0; a1 < (1L << q); a1++) 
            B[(c1 << (k1+q)) + (b1 << q) + a1] = T[(a1 << q) + c];
      }
   }
}

static
void BitReverseCopy(unsigned long * NTL_RESTRICT B, const long * NTL_RESTRICT A, long k)
{
   if (k <= NTL_BRC_THRESH) 
      BasicBitReverseCopy(B, A, k);
   else
      COBRA(B, A, k);
}




#endif


#ifdef NTL_FFT_LAZYMUL 
// we only honor the FFT_LAZYMUL flag if either the SPMM_ULL, SPMM_ASM, or LONGLONG_SP_MULMOD 
// flags are set

#if (!defined(NTL_SPMM_ULL) && !defined(NTL_SPMM_ASM) && !defined(NTL_LONGLONG_SP_MULMOD))
#undef NTL_FFT_LAZYMUL
#endif

#endif

#ifndef NTL_FFT_BIGTAB

#define NTL_FFT_ROUTINE_TAB     FFT_aux
#define NTL_FFT_ROUTINE_NOTAB   FFT

#else

#define NTL_FFT_ROUTINE_TAB     FFT
#define NTL_FFT_ROUTINE_NOTAB   FFT_aux

#endif






#ifndef NTL_FFT_LAZYMUL


// A basic FFT, no tables, no lazy strategy

void NTL_FFT_ROUTINE_NOTAB(long* A, const long* a, long k, const FFTPrimeInfo& info, long dir)
// performs a 2^k-point convolution modulo q

{
   long q = info.q;
   const long *root = info.RootTable[dir].elts();
   mulmod_t qinv = info.qinv;
   
   if (k <= 1) {
      if (k == 0) {
	 A[0] = a[0];
	 return;
      }
      if (k == 1) {
	 long a0 = AddMod(a[0], a[1], q);
	 long a1 = SubMod(a[0], a[1], q);
         A[0] = a0;
         A[1] = a1;
	 return;
      }
   }

   // assume k > 1

   NTL_TLS_LOCAL(Vec<long>, wtab_store);
   NTL_TLS_LOCAL(Vec<mulmod_precon_t>, wqinvtab_store);
   NTL_TLS_LOCAL(Vec<long>, AA_store);

   wtab_store.SetLength(1L << (k-2));
   wqinvtab_store.SetLength(1L << (k-2));
   AA_store.SetLength(1L << k);

   long * NTL_RESTRICT wtab = wtab_store.elts();
   mulmod_precon_t * NTL_RESTRICT wqinvtab = wqinvtab_store.elts();
   long *AA = AA_store.elts();

   wtab[0] = 1;
   wqinvtab[0] = PrepMulModPrecon(1, q, qinv);


   BitReverseCopy(AA, a, k);

   long n = 1L << k;

   long s, m, m_half, m_fourth, i, j, t, u, t1, u1, tt, tt1;

   long w;
   mulmod_precon_t wqinv;

   // s = 1

   for (i = 0; i < n; i += 2) {
      t = AA[i + 1];
      u = AA[i];
      AA[i] = AddMod(u, t, q);
      AA[i+1] = SubMod(u, t, q);
   }

   
  
   for (s = 2; s < k; s++) {
      m = 1L << s;
      m_half = 1L << (s-1);
      m_fourth = 1L << (s-2);

      w = root[s];
      wqinv = PrepMulModPrecon(w, q, qinv);

      // prepare wtab...

#if 1
      // plain version...

      for (i = m_half-1, j = m_fourth-1; i >= 0; i -= 2, j--) {
         long w_j = wtab[j];
         mulmod_precon_t wqi_j = wqinvtab[j];
         long w_i = MulModPrecon(w_j, w, q, wqinv);
         mulmod_precon_t wqi_i = PrepMulModPrecon(w_i, q, qinv);

         wtab[i-1] = w_j;
         wqinvtab[i-1] = wqi_j;
         wtab[i] = w_i;
         wqinvtab[i] = wqi_i;
      }
#else
      // software pipeline version...doesn't seem to make a big difference

      if (s == 2) {
         wtab[1] = MulModPrecon(wtab[0], w, q, wqinv);
         wqinvtab[1] = PrepMulModPrecon(wtab[1], q, qinv);
      }
      else {
         i = m_half-1; j = m_fourth-1;
         wtab[i-1] = wtab[j];
         wqinvtab[i-1] = wqinvtab[j];
         wtab[i] = MulModPrecon(wtab[i-1], w, q, wqinv);

         i -= 2; j --;

         for (; i >= 0; i -= 2, j --) {
            long wp2 = wtab[i+2];
            long wm1 = wtab[j];
            wqinvtab[i+2] = PrepMulModPrecon(wp2, q, qinv);
            wtab[i-1] = wm1;
            wqinvtab[i-1] = wqinvtab[j];
            wtab[i] = MulModPrecon(wm1, w, q, wqinv);
         }

         wqinvtab[1] = PrepMulModPrecon(wtab[1], q, qinv);
      }


#endif


      for (i = 0; i < n; i+= m) {

         long * NTL_RESTRICT AA0 = &AA[i];
         long * NTL_RESTRICT AA1 = &AA[i + m_half];



#if 1
         // loop unrolling and pipelining
          
         t = AA1[0];
         u = AA0[0];
         t1 = MulModPrecon(AA1[1], w, q, wqinv);
         u1 = AA0[1];



         for (j = 0; j < m_half-2; j += 2) {
            long a02 = AA0[j+2];
            long a03 = AA0[j+3];
            long a12 = AA1[j+2];
            long a13 = AA1[j+3];
            long w2 = wtab[j+2];
            long w3 = wtab[j+3];
            mulmod_precon_t wqi2 = wqinvtab[j+2];
            mulmod_precon_t wqi3 = wqinvtab[j+3];

            tt = MulModPrecon(a12, w2, q, wqi2);
            long b00 = AddMod(u, t, q);
            long b10 = SubMod(u, t, q);
            t = tt;
            u = a02;

            tt1 = MulModPrecon(a13, w3, q, wqi3);
            long b01 = AddMod(u1, t1, q);
            long b11 = SubMod(u1, t1, q);
            t1 = tt1;
            u1 = a03;

            AA0[j] = b00;
            AA1[j] = b10;
            AA0[j+1] = b01;
            AA1[j+1] = b11;
         }


         AA0[j] = AddMod(u, t, q);
         AA1[j] = SubMod(u, t, q);
         AA0[j + 1] = AddMod(u1, t1, q);
         AA1[j + 1] = SubMod(u1, t1, q);


#else
         // no loop unrolling, but still some pipelining

          
         t = AA1[0];
         u = AA0[0];

         for (j = 0; j < m_half-1; j++) {
            long a02 = AA0[j+1];
            long a12 = AA1[j+1];
            long w2 = wtab[j+1];
            mulmod_precon_t wqi2 = wqinvtab[j+1];

            tt = MulModPrecon(a12, w2, q, wqi2);
            long b00 = AddMod(u, t, q);
            long b10 = SubMod(u, t, q);
            t = tt;
            u = a02;

            AA0[j] = b00;
            AA1[j] = b10;
         }


         AA0[j] = AddMod(u, t, q);
         AA1[j] = SubMod(u, t, q);


#endif
      }
   }


   // s == k...special case

   m = 1L << s;
   m_half = 1L << (s-1);
   m_fourth = 1L << (s-2);


   w = root[s];
   wqinv = PrepMulModPrecon(w, q, qinv);

   // j = 0, 1

   t = AA[m_half];
   u = AA[0];
   t1 = MulModPrecon(AA[1+ m_half], w, q, wqinv);
   u1 = AA[1];

   A[0] = AddMod(u, t, q);
   A[m_half] = SubMod(u, t, q);
   A[1] = AddMod(u1, t1, q);
   A[1 + m_half] = SubMod(u1, t1, q);

   for (j = 2; j < m_half; j += 2) {
      t = MulModPrecon(AA[j + m_half], wtab[j >> 1], q, wqinvtab[j >> 1]);
      u = AA[j];
      t1 = MulModPrecon(AA[j + 1+ m_half], wtab[j >> 1], q, 
                        wqinvtab[j >> 1]);
      t1 = MulModPrecon(t1, w, q, wqinv);
      u1 = AA[j + 1];

      A[j] = AddMod(u, t, q);
      A[j + m_half] = SubMod(u, t, q);
      A[j + 1] = AddMod(u1, t1, q);
      A[j + 1 + m_half] = SubMod(u1, t1, q);
     
   }
}







#else



// FFT with  lazy multiplication

#if (defined(NTL_LONGLONG_SP_MULMOD))


#if (NTL_BITS_PER_LONG >= NTL_SP_NBITS+4) 

static inline unsigned long 
sp_NormalizedLazyPrepMulModPreconWithRem(unsigned long& rres, long b, long n, unsigned long ninv)
{
   unsigned long H = cast_unsigned(b);
   unsigned long Q = MulHiUL(H << 4, ninv);
   unsigned long L = cast_unsigned(b) << (NTL_SP_NBITS+2);
   long r = L - Q*cast_unsigned(n);  // r in [0..2*n)

   r = sp_CorrectExcessQuo(Q, r, n);
   rres = r;
   return Q; // NOTE: not shifted
}

static inline unsigned long 
sp_NormalizedLazyPrepMulModPrecon(long b, long n, unsigned long ninv)
{
   unsigned long H = cast_unsigned(b);
   unsigned long Q = MulHiUL(H << 4, ninv);
   unsigned long L = cast_unsigned(b) << (NTL_SP_NBITS+2);
   long r = L - Q*cast_unsigned(n);  // r in [0..2*n)

   Q += 1L + sp_SignMask(r-n);
   return Q; // NOTE: not shifted
}


#else

// NTL_BITS_PER_LONG == NTL_SP_NBITS+2
static inline unsigned long 
sp_NormalizedLazyPrepMulModPreconWithRem(unsigned long& rres, long b, long n, unsigned long ninv)
{
   unsigned long H = cast_unsigned(b) << 2;
   unsigned long Q = MulHiUL(H, (ninv << 1)) + H;
   unsigned long rr = -Q*cast_unsigned(n);  // r in [0..3*n)

   long r = sp_CorrectExcessQuo(Q, rr, n);
   r = sp_CorrectExcessQuo(Q, r, n);
   rres = r;
   return Q;  // NOTE: not shifted
}

static inline unsigned long 
sp_NormalizedLazyPrepMulModPrecon(long b, long n, unsigned long ninv)
{
   unsigned long H = cast_unsigned(b) << 2;
   unsigned long Q = MulHiUL(H, (ninv << 1)) + H;
   unsigned long rr = -Q*cast_unsigned(n);  // r in [0..3*n)
   Q += 2L + sp_SignMask(rr-n) + sp_SignMask(rr-2*n);
   return Q; // NOTE: not shifted
}


#endif


static inline unsigned long
LazyPrepMulModPrecon(long b, long n, sp_inverse ninv)
{
   return sp_NormalizedLazyPrepMulModPrecon(b << ninv.shamt, n << ninv.shamt, ninv.inv) << (NTL_BITS_PER_LONG-NTL_SP_NBITS-2);
}


static inline unsigned long
LazyPrepMulModPreconWithRem(unsigned long& rres, long b, long n, sp_inverse ninv)
{
   unsigned long qq, rr;
   qq = sp_NormalizedLazyPrepMulModPreconWithRem(rr, b << ninv.shamt, n << ninv.shamt, ninv.inv); 
   rres = rr >> ninv.shamt;
   return qq << (NTL_BITS_PER_LONG-NTL_SP_NBITS-2);
}








#elif (NTL_BITS_PER_LONG - NTL_SP_NBITS >= 4 && NTL_WIDE_DOUBLE_PRECISION - NTL_SP_NBITS >= 4)


// slightly faster functions, which should kick in on x86-64, where 
//    NTL_BITS_PER_LONG == 64
//    NTL_SP_NBITS == 60 (another reason for holding this back to 60 bits)
//    NTL_WIDE_DOUBLE_PRECISION == 64

// DIRT: if the relative error in floating point calcuations (muls and reciprocals)
//   is <= epsilon, the relative error in the calculations is <= 3*epsilon +
//   O(epsilon^2), and we require that this relative error is at most
//   2^{-(NTL_SP_NBITS+2)}, so it should be pretty safe as long as
//   epsilon is at most, or not much geater than, 2^{-NTL_WIDE_DOUBLE_PRECISION}.

static inline 
unsigned long LazyPrepMulModPrecon(long b, long n, wide_double ninv)
{
   long q = (long) ( (((wide_double) b) * wide_double(4*NTL_SP_BOUND)) * ninv ); 

   unsigned long rr = (cast_unsigned(b) << (NTL_SP_NBITS+2)) 
                       - cast_unsigned(q)*cast_unsigned(n);

   q += sp_SignMask(rr) + sp_SignMask(rr-n) + 1L;

   return cast_unsigned(q) << (NTL_BITS_PER_LONG - NTL_SP_NBITS - 2);
}

static inline 
unsigned long LazyPrepMulModPreconWithRem(unsigned long& rres, long b, long n, wide_double ninv)
{
   long q = (long) ( (((wide_double) b) * wide_double(4*NTL_SP_BOUND)) * ninv ); 

   unsigned long rr = (cast_unsigned(b) << (NTL_SP_NBITS+2)) 
                       - cast_unsigned(q)*cast_unsigned(n);

   long r = sp_CorrectDeficitQuo(q, rr, n);
   r = sp_CorrectExcessQuo(q, r, n);

   unsigned long qres = cast_unsigned(q) << (NTL_BITS_PER_LONG - NTL_SP_NBITS - 2);
   rres = r;
   return qres;
}

#else


static inline 
unsigned long LazyPrepMulModPrecon(long b, long n, wide_double ninv)
{
   long q = (long) ( (((wide_double) b) * wide_double(NTL_SP_BOUND)) * ninv ); 

   unsigned long rr = (cast_unsigned(b) << (NTL_SP_NBITS)) 
                       - cast_unsigned(q)*cast_unsigned(n);

   long r = sp_CorrectDeficitQuo(q, rr, n);
   r = sp_CorrectExcessQuo(q, r, n);

   unsigned long qq = q;

   qq = 2*qq;
   r = 2*r;
   r = sp_CorrectExcessQuo(qq, r, n);

   qq = 2*qq;
   r = 2*r;
   qq += sp_SignMask(r-n) + 1L;

   return qq << (NTL_BITS_PER_LONG - NTL_SP_NBITS - 2);
}





static inline 
unsigned long LazyPrepMulModPreconWithRem(unsigned long& rres, long b, long n, wide_double ninv)
{
   long q = (long) ( (((wide_double) b) * wide_double(NTL_SP_BOUND)) * ninv ); 

   unsigned long rr = (cast_unsigned(b) << (NTL_SP_NBITS)) 
                       - cast_unsigned(q)*cast_unsigned(n);

   long r = sp_CorrectDeficitQuo(q, rr, n);
   r = sp_CorrectExcessQuo(q, r, n);

   unsigned long qq = q;

   qq = 2*qq;
   r = 2*r;
   r = sp_CorrectExcessQuo(qq, r, n);

   qq = 2*qq;
   r = 2*r;
   r = sp_CorrectExcessQuo(qq, r, n);

   rres = r;
   return qq << (NTL_BITS_PER_LONG - NTL_SP_NBITS - 2);
}

#endif



static inline
unsigned long LazyMulModPreconQuo(unsigned long a, unsigned long b, 
                                  unsigned long n, unsigned long bninv)
{
   unsigned long q = MulHiUL(a, bninv);
   unsigned long r = a*b - q*n;
   q += sp_SignMask(r-n) + 1L;
   return q << (NTL_BITS_PER_LONG - NTL_SP_NBITS - 2);
}


static inline 
unsigned long LazyMulModPrecon(unsigned long a, unsigned long b, 
                               unsigned long n, unsigned long bninv)
{
   unsigned long q = MulHiUL(a, bninv);
   unsigned long res = a*b - q*n;
   return res;
}


static inline 
unsigned long LazyReduce1(unsigned long a, long q)
{
  return sp_CorrectExcess(long(a), q);
}

static inline 
unsigned long LazyReduce2(unsigned long a, long q)
{
  return sp_CorrectExcess(a, 2*q);
}




// FFT: Lazy, no tables

void NTL_FFT_ROUTINE_NOTAB(long* A, const long* a, long k, const FFTPrimeInfo& info, long dir)

// performs a 2^k-point convolution modulo q

{
   long q = info.q;
   const long *root = info.RootTable[dir].elts();
   mulmod_t qinv = info.qinv;

   if (k <= 1) {
      if (k == 0) {
	 A[0] = a[0];
	 return;
      }
      if (k == 1) {
	 long a0 = AddMod(a[0], a[1], q);
	 long a1 = SubMod(a[0], a[1], q);
         A[0] = a0;
         A[1] = a1;
	 return;
      }
   }

   // assume k >= 2

   NTL_TLS_LOCAL(Vec<unsigned long>, AA_store);
   AA_store.SetLength(1L << k);
   unsigned long *AA = AA_store.elts();

   NTL_TLS_LOCAL(Vec<long>, wtab_store);
   wtab_store.SetLength(max(2, 1L << (k-2))); 
   // allocate space for at least 2 elements, to deal with a corner case when k == 2
   long * NTL_RESTRICT wtab = wtab_store.elts();

   NTL_TLS_LOCAL(Vec<mulmod_precon_t>, wqinvtab_store);
   wqinvtab_store.SetLength(max(2, 1L << (k-2)));
   // allocate space for at least 2 elements, to deal with a corner case when k == 2
   mulmod_precon_t * NTL_RESTRICT wqinvtab = wqinvtab_store.elts();


   BitReverseCopy(AA, a, k);

   long n = 1L << k;


   /* we work with redundant representations, in the range [0, 4q) */

   long s, m, m_half, m_fourth, i, j; 
   unsigned long t, u, t1, u1;


   wtab[0] = 1;
   wqinvtab[0] = LazyPrepMulModPrecon(1, q, qinv);

   // s = 1
   for (i = 0; i < n; i += 2) {
      t = AA[i + 1];
      u = AA[i];
      AA[i] = u + t;
      AA[i+1] = u - t + q;
   }

   // s = 2
   {
      long w = root[2];
      mulmod_precon_t wqinv = LazyPrepMulModPrecon(w, q, qinv);

      wtab[1] = w;
      wqinvtab[1] = wqinv;


      for (i = 0; i < n; i += 4) {

         unsigned long * NTL_RESTRICT AA0 = &AA[i];
         unsigned long * NTL_RESTRICT AA1 = &AA[i + 2];

         {
            const unsigned long a11 = AA1[0];
            const unsigned long a01 = AA0[0];

            const unsigned long tt1 = a11; 
            const unsigned long uu1 = a01; 
            const unsigned long b01 = uu1 + tt1; 
            const unsigned long b11 = uu1 - tt1 + 2*q;

            AA0[0] = b01;
            AA1[0] = b11;
         }
         {
            const unsigned long a11 = AA1[1];
            const unsigned long a01 = AA0[1];

            const unsigned long tt1 = LazyMulModPrecon(a11, w, q, wqinv); 
            const unsigned long uu1 = a01; 
            const unsigned long b01 = uu1 + tt1; 
            const unsigned long b11 = uu1 - tt1 + 2*q;

            AA0[1] = b01;
            AA1[1] = b11;
         }
      }
   }


   //  s = 3..k-1

   for (s = 3; s < k; s++) {
      m = 1L << s;
      m_half = 1L << (s-1);
      m_fourth = 1L << (s-2);

      long w = root[s];

#if 0
      // This computes all the multipliers in a straightforward fashion.
      // It's a bit slower that the strategy used below, even if
      // NTL_LONGLONG_SP_MULMOD is set

      mulmod_precon_t wqinv = LazyPrepMulModPrecon(w, q, qinv);


      for (i = m_half-1, j = m_fourth-1; i >= 0; i -= 2, j--) {
         long w_j = wtab[j];
         mulmod_precon_t wqi_j = wqinvtab[j];

         long w_i = LazyReduce1(LazyMulModPrecon(w_j, w, q, wqinv), q);
         mulmod_precon_t wqi_i = LazyPrepMulModPrecon(w_i, q, qinv);

         wtab[i-1] = w_j;
         wqinvtab[i-1] = wqi_j;
         wtab[i] = w_i;
         wqinvtab[i] = wqi_i;
      }
#else
      unsigned long wqinv_rem;
      mulmod_precon_t wqinv = LazyPrepMulModPreconWithRem(wqinv_rem, w, q, qinv);


      for (i = m_half-1, j = m_fourth-1; i >= 0; i -= 2, j--) {
         long w_j = wtab[j];
         mulmod_precon_t wqi_j = wqinvtab[j];

         // The next two lines are equivalent, but the first involves
         // a computation of hi(w_j*wqinv), which pairs with the
         // computation of lo(w_j*wqinv) below...but I don't think
         // the compiler sees this...oh well...

         long w_i = LazyReduce1(LazyMulModPrecon(w_j, w, q, wqinv), q);
         // long w_i = LazyReduce1(LazyMulModPrecon(w, w_j, q, wqi_j), q);

         mulmod_precon_t wqi_i = LazyMulModPreconQuo(wqinv_rem, w_j, q, wqi_j) 
                                   + cast_unsigned(w_j)*wqinv;

         wtab[i-1] = w_j;
         wqinvtab[i-1] = wqi_j;
         wtab[i] = w_i;
         wqinvtab[i] = wqi_i;
      }


#endif

      for (i = 0; i < n; i += m) {

         unsigned long * NTL_RESTRICT AA0 = &AA[i];
         unsigned long * NTL_RESTRICT AA1 = &AA[i + m_half];


         for (j = 0; j < m_half; j += 4) {
            {
               const long w1 = wtab[j+0];
               const mulmod_precon_t wqi1 = wqinvtab[j+0];
               const unsigned long a11 = AA1[j+0];
               const unsigned long a01 = AA0[j+0];

               const unsigned long tt1 = LazyMulModPrecon(a11, w1, q, wqi1);
               const unsigned long uu1 = LazyReduce2(a01, q);
               const unsigned long b01 = uu1 + tt1; 
               const unsigned long b11 = uu1 - tt1 + 2*q;

               AA0[j+0] = b01;
               AA1[j+0] = b11;
            }
            {
               const long w1 = wtab[j+1];
               const mulmod_precon_t wqi1 = wqinvtab[j+1];
               const unsigned long a11 = AA1[j+1];
               const unsigned long a01 = AA0[j+1];

               const unsigned long tt1 = LazyMulModPrecon(a11, w1, q, wqi1);
               const unsigned long uu1 = LazyReduce2(a01, q);
               const unsigned long b01 = uu1 + tt1; 
               const unsigned long b11 = uu1 - tt1 + 2*q;

               AA0[j+1] = b01;
               AA1[j+1] = b11;
            }
            {
               const long w1 = wtab[j+2];
               const mulmod_precon_t wqi1 = wqinvtab[j+2];
               const unsigned long a11 = AA1[j+2];
               const unsigned long a01 = AA0[j+2];

               const unsigned long tt1 = LazyMulModPrecon(a11, w1, q, wqi1);
               const unsigned long uu1 = LazyReduce2(a01, q);
               const unsigned long b01 = uu1 + tt1; 
               const unsigned long b11 = uu1 - tt1 + 2*q;

               AA0[j+2] = b01;
               AA1[j+2] = b11;
            }
            {
               const long w1 = wtab[j+3];
               const mulmod_precon_t wqi1 = wqinvtab[j+3];
               const unsigned long a11 = AA1[j+3];
               const unsigned long a01 = AA0[j+3];

               const unsigned long tt1 = LazyMulModPrecon(a11, w1, q, wqi1);
               const unsigned long uu1 = LazyReduce2(a01, q);
               const unsigned long b01 = uu1 + tt1; 
               const unsigned long b11 = uu1 - tt1 + 2*q;

               AA0[j+3] = b01;
               AA1[j+3] = b11;
            }
         }
      }
   }



   // special case: s == k to avoid extraneous computation of constants

   if (k > 2) { 
      s = k;

      m = 1L << s;
      m_half = 1L << (s-1);
      m_fourth = 1L << (s-2);

      long w = root[s];
      mulmod_precon_t wqinv = LazyPrepMulModPrecon(w, q, qinv);


      for (i = 0; i < n; i += m) {

         unsigned long * NTL_RESTRICT AA0 = &AA[i];
         unsigned long * NTL_RESTRICT AA1 = &AA[i + m_half];

         long half_j;

         for (j = 0, half_j = 0; j < m_half; j += 4, half_j += 2) {
            {
               const long w1 = wtab[half_j+0];
               const mulmod_precon_t wqi1 = wqinvtab[half_j+0];
               const unsigned long a11 = AA1[j+0];
               const unsigned long a01 = AA0[j+0];

               const unsigned long tt1 = LazyMulModPrecon(a11, w1, q, wqi1);
               const unsigned long uu1 = LazyReduce2(a01, q);
               const unsigned long b01 = uu1 + tt1; 
               const unsigned long b11 = uu1 - tt1 + 2*q;

               AA0[j+0] = b01;
               AA1[j+0] = b11;
            }
            {
               const long w1 = wtab[half_j+0];
               const mulmod_precon_t wqi1 = wqinvtab[half_j+0];
               const unsigned long a11 = AA1[j+1];
               const unsigned long a01 = AA0[j+1];

               const unsigned long tt1 = LazyMulModPrecon(LazyMulModPrecon(a11, w1, q, wqi1),
                                                          w, q, wqinv);
               const unsigned long uu1 = LazyReduce2(a01, q);
               const unsigned long b01 = uu1 + tt1; 
               const unsigned long b11 = uu1 - tt1 + 2*q;

               AA0[j+1] = b01;
               AA1[j+1] = b11;
            }
            {
               const long w1 = wtab[half_j+1];
               const mulmod_precon_t wqi1 = wqinvtab[half_j+1];
               const unsigned long a11 = AA1[j+2];
               const unsigned long a01 = AA0[j+2];

               const unsigned long tt1 = LazyMulModPrecon(a11, w1, q, wqi1);
               const unsigned long uu1 = LazyReduce2(a01, q);
               const unsigned long b01 = uu1 + tt1; 
               const unsigned long b11 = uu1 - tt1 + 2*q;

               AA0[j+2] = b01;
               AA1[j+2] = b11;
            }
            {
               const long w1 = wtab[half_j+1];
               const mulmod_precon_t wqi1 = wqinvtab[half_j+1];
               const unsigned long a11 = AA1[j+3];
               const unsigned long a01 = AA0[j+3];

               const unsigned long tt1 = LazyMulModPrecon(LazyMulModPrecon(a11, w1, q, wqi1),
                                                          w, q, wqinv);
               const unsigned long uu1 = LazyReduce2(a01, q);
               const unsigned long b01 = uu1 + tt1; 
               const unsigned long b11 = uu1 - tt1 + 2*q;

               AA0[j+3] = b01;
               AA1[j+3] = b11;
            }
         }
      }
   }


   /* need to reduce redundant representations */

   for (i = 0; i < n; i++) {
      unsigned long tmp = LazyReduce2(AA[i], q);
      A[i] = LazyReduce1(tmp, q);
   }
}


#endif







#ifndef NTL_FFT_LAZYMUL

// FFT with precomputed tables, no lazy mul 

static
void PrecompFFTMultipliers(long k, long q, mulmod_t qinv, const long *root, const FFTMultipliers& tab)
{
   if (k < 1) LogicError("PrecompFFTMultipliers: bad input");

   do {  // NOTE: thread safe lazy init
      FFTMultipliers::Builder bld(tab, k+1);
      long amt = bld.amt();
      if (!amt) break;

      long first = k+1-amt;
      // initialize entries first..k


      for (long s = first; s <= k; s++) {
         UniquePtr<FFTVectorPair> item;

         if (s == 0) {
            bld.move(item); // position 0 not used
            continue;
         }

         if (s == 1) {
            item.make();
            item->wtab_precomp.SetLength(1);
            item->wqinvtab_precomp.SetLength(1);
            item->wtab_precomp[0] = 1;
            item->wqinvtab_precomp[0] = PrepMulModPrecon(1, q, qinv);
            bld.move(item);
            continue;
         }

         item.make();
         item->wtab_precomp.SetLength(1L << (s-1));
         item->wqinvtab_precomp.SetLength(1L << (s-1));

         long m = 1L << s;
         long m_half = 1L << (s-1);
         long m_fourth = 1L << (s-2);

         const long *wtab_last = tab[s-1]->wtab_precomp.elts();
         const mulmod_precon_t *wqinvtab_last = tab[s-1]->wqinvtab_precomp.elts();

         long *wtab = item->wtab_precomp.elts();
         mulmod_precon_t *wqinvtab = item->wqinvtab_precomp.elts();

         for (long i = 0; i < m_fourth; i++) {
            wtab[i] = wtab_last[i];
            wqinvtab[i] = wqinvtab_last[i];
         } 

         long w = root[s];
         mulmod_precon_t wqinv = PrepMulModPrecon(w, q, qinv);

         // prepare wtab...

         if (s == 2) {
            wtab[1] = MulModPrecon(wtab[0], w, q, wqinv);
            wqinvtab[1] = PrepMulModPrecon(wtab[1], q, qinv);
         }
         else {
            // some software pipelining
            long i, j;

            i = m_half-1; j = m_fourth-1;
            wtab[i-1] = wtab[j];
            wqinvtab[i-1] = wqinvtab[j];
            wtab[i] = MulModPrecon(wtab[i-1], w, q, wqinv);

            i -= 2; j --;

            for (; i >= 0; i -= 2, j --) {
               long wp2 = wtab[i+2];
               long wm1 = wtab[j];
               wqinvtab[i+2] = PrepMulModPrecon(wp2, q, qinv);
               wtab[i-1] = wm1;
               wqinvtab[i-1] = wqinvtab[j];
               wtab[i] = MulModPrecon(wm1, w, q, wqinv);
            }

            wqinvtab[1] = PrepMulModPrecon(wtab[1], q, qinv);
         }

         bld.move(item);
      }
   } while (0);
}


// FFT: no lazy, table

void NTL_FFT_ROUTINE_TAB(long* A, const long* a, long k, const FFTPrimeInfo& info, long dir)
// performs a 2^k-point convolution modulo q

{
   if (!info.bigtab || k > NTL_FFT_BIGTAB_MAXROOT) {
      NTL_FFT_ROUTINE_NOTAB(A, a, k, info, dir);
      return;
   }


   long q = info.q;
   const long *root = info.RootTable[dir].elts();
   mulmod_t qinv = info.qinv;
   const FFTMultipliers& tab = info.bigtab->MulTab[dir];


   if (k <= 1) {
      if (k == 0) {
	 A[0] = a[0];
	 return;
      }
      if (k == 1) {
	 long a0 = AddMod(a[0], a[1], q);
	 long a1 = SubMod(a[0], a[1], q);
         A[0] = a0;
         A[1] = a1;
	 return;
      }
   }

   // assume k > 1

   if (k >= tab.length()) PrecompFFTMultipliers(k, q, qinv, root, tab);

   NTL_TLS_LOCAL(Vec<long>, AA_store);
   AA_store.SetLength(1L << k);
   long *AA = AA_store.elts();

   BitReverseCopy(AA, a, k);

   long n = 1L << k;

   long s, m, m_half, m_fourth, i, j, t, u, t1, u1, tt, tt1;

   // s = 1

   for (i = 0; i < n; i += 2) {
      t = AA[i + 1];
      u = AA[i];
      AA[i] = AddMod(u, t, q);
      AA[i+1] = SubMod(u, t, q);
   }
   
  
   for (s = 2; s < k; s++) {
      m = 1L << s;
      m_half = 1L << (s-1);
      m_fourth = 1L << (s-2);

      const long* NTL_RESTRICT wtab = tab[s]->wtab_precomp.elts();
      const mulmod_precon_t * NTL_RESTRICT wqinvtab = tab[s]->wqinvtab_precomp.elts();

      for (i = 0; i < n; i+= m) {

         long * NTL_RESTRICT AA0 = &AA[i];
         long * NTL_RESTRICT AA1 = &AA[i + m_half];

#ifdef NTL_PIPELINE

// pipelining: seems to be faster
          
         t = AA1[0];
         u = AA0[0];
         t1 = MulModPrecon(AA1[1], wtab[1], q, wqinvtab[1]);
         u1 = AA0[1];

         for (j = 0; j < m_half-2; j += 2) {
            long a02 = AA0[j+2];
            long a03 = AA0[j+3];
            long a12 = AA1[j+2];
            long a13 = AA1[j+3];
            long w2 = wtab[j+2];
            long w3 = wtab[j+3];
            mulmod_precon_t wqi2 = wqinvtab[j+2];
            mulmod_precon_t wqi3 = wqinvtab[j+3];

            tt = MulModPrecon(a12, w2, q, wqi2);
            long b00 = AddMod(u, t, q);
            long b10 = SubMod(u, t, q);

            tt1 = MulModPrecon(a13, w3, q, wqi3);
            long b01 = AddMod(u1, t1, q);
            long b11 = SubMod(u1, t1, q);

            AA0[j] = b00;
            AA1[j] = b10;
            AA0[j+1] = b01;
            AA1[j+1] = b11;


            t = tt;
            u = a02;
            t1 = tt1;
            u1 = a03;
         }


         AA0[j] = AddMod(u, t, q);
         AA1[j] = SubMod(u, t, q);
         AA0[j + 1] = AddMod(u1, t1, q);
         AA1[j + 1] = SubMod(u1, t1, q);
      }
#else
         for (j = 0; j < m_half; j += 2) {
            const long a00 = AA0[j];
            const long a01 = AA0[j+1];
            const long a10 = AA1[j];
            const long a11 = AA1[j+1];

            const long w0 = wtab[j];
            const long w1 = wtab[j+1];
            const mulmod_precon_t wqi0 = wqinvtab[j];
            const mulmod_precon_t wqi1 = wqinvtab[j+1];

            const long tt = MulModPrecon(a10, w0, q, wqi0);
            const long uu = a00;
            const long b00 = AddMod(uu, tt, q); 
            const long b10 = SubMod(uu, tt, q);

            const long tt1 = MulModPrecon(a11, w1, q, wqi1);
            const long uu1 = a01;
            const long b01 = AddMod(uu1, tt1, q); 
            const long b11 = SubMod(uu1, tt1, q);

            AA0[j] = b00;
            AA0[j+1] = b01;
            AA1[j] = b10;
            AA1[j+1] = b11;
         }
      }
#endif
   }


   // s == k, special case
   {
      m = 1L << s;
      m_half = 1L << (s-1);
      m_fourth = 1L << (s-2);

      const long* NTL_RESTRICT wtab = tab[s]->wtab_precomp.elts();
      const mulmod_precon_t * NTL_RESTRICT wqinvtab = tab[s]->wqinvtab_precomp.elts();

      for (i = 0; i < n; i+= m) {

         long * NTL_RESTRICT AA0 = &AA[i];
         long * NTL_RESTRICT AA1 = &AA[i + m_half];
         long * NTL_RESTRICT A0 = &A[i];
         long * NTL_RESTRICT A1 = &A[i + m_half];

#ifdef NTL_PIPELINE

// pipelining: seems to be faster
          
         t = AA1[0];
         u = AA0[0];
         t1 = MulModPrecon(AA1[1], wtab[1], q, wqinvtab[1]);
         u1 = AA0[1];

         for (j = 0; j < m_half-2; j += 2) {
            long a02 = AA0[j+2];
            long a03 = AA0[j+3];
            long a12 = AA1[j+2];
            long a13 = AA1[j+3];
            long w2 = wtab[j+2];
            long w3 = wtab[j+3];
            mulmod_precon_t wqi2 = wqinvtab[j+2];
            mulmod_precon_t wqi3 = wqinvtab[j+3];

            tt = MulModPrecon(a12, w2, q, wqi2);
            long b00 = AddMod(u, t, q);
            long b10 = SubMod(u, t, q);

            tt1 = MulModPrecon(a13, w3, q, wqi3);
            long b01 = AddMod(u1, t1, q);
            long b11 = SubMod(u1, t1, q);

            A0[j] = b00;
            A1[j] = b10;
            A0[j+1] = b01;
            A1[j+1] = b11;


            t = tt;
            u = a02;
            t1 = tt1;
            u1 = a03;
         }


         A0[j] = AddMod(u, t, q);
         A1[j] = SubMod(u, t, q);
         A0[j + 1] = AddMod(u1, t1, q);
         A1[j + 1] = SubMod(u1, t1, q);
      }
#else
         for (j = 0; j < m_half; j += 2) {
            const long a00 = AA0[j];
            const long a01 = AA0[j+1];
            const long a10 = AA1[j];
            const long a11 = AA1[j+1];

            const long w0 = wtab[j];
            const long w1 = wtab[j+1];
            const mulmod_precon_t wqi0 = wqinvtab[j];
            const mulmod_precon_t wqi1 = wqinvtab[j+1];

            const long tt = MulModPrecon(a10, w0, q, wqi0);
            const long uu = a00;
            const long b00 = AddMod(uu, tt, q); 
            const long b10 = SubMod(uu, tt, q);

            const long tt1 = MulModPrecon(a11, w1, q, wqi1);
            const long uu1 = a01;
            const long b01 = AddMod(uu1, tt1, q); 
            const long b11 = SubMod(uu1, tt1, q);

            A0[j] = b00;
            A0[j+1] = b01;
            A1[j] = b10;
            A1[j+1] = b11;
         }
      }
#endif
   }

}






#else

// FFT with precomputed tables, lazy mul 


static
void LazyPrecompFFTMultipliers(long k, long q, mulmod_t qinv, const long *root, const FFTMultipliers& tab)
{
   if (k < 1) LogicError("LazyPrecompFFTMultipliers: bad input");

   do { // NOTE: thread safe lazy init
      FFTMultipliers::Builder bld(tab, k+1);
      long amt = bld.amt();
      if (!amt) break;

      long first = k+1-amt;
      // initialize entries first..k


      for (long s = first; s <= k; s++) {
         UniquePtr<FFTVectorPair> item;

         if (s == 0) {
            bld.move(item); // position 0 not used
            continue;
         }

         if (s == 1) {
            item.make();
            item->wtab_precomp.SetLength(1);
            item->wqinvtab_precomp.SetLength(1);
            item->wtab_precomp[0] = 1;
            item->wqinvtab_precomp[0] = LazyPrepMulModPrecon(1, q, qinv);
            bld.move(item);
            continue;
         }

         item.make();
         item->wtab_precomp.SetLength(1L << (s-1));
         item->wqinvtab_precomp.SetLength(1L << (s-1));

         long m = 1L << s;
         long m_half = 1L << (s-1);
         long m_fourth = 1L << (s-2);

         const long *wtab_last = tab[s-1]->wtab_precomp.elts();
         const mulmod_precon_t *wqinvtab_last = tab[s-1]->wqinvtab_precomp.elts();

         long *wtab = item->wtab_precomp.elts();
         mulmod_precon_t *wqinvtab = item->wqinvtab_precomp.elts();

         for (long i = 0; i < m_fourth; i++) {
            wtab[i] = wtab_last[i];
            wqinvtab[i] = wqinvtab_last[i];
         } 

         long w = root[s];
         mulmod_precon_t wqinv = LazyPrepMulModPrecon(w, q, qinv);

         // prepare wtab...

         if (s == 2) {
            wtab[1] = LazyReduce1(LazyMulModPrecon(wtab[0], w, q, wqinv), q);
            wqinvtab[1] = LazyPrepMulModPrecon(wtab[1], q, qinv);
         }
         else {
            // some software pipelining
            long i, j;

            i = m_half-1; j = m_fourth-1;
            wtab[i-1] = wtab[j];
            wqinvtab[i-1] = wqinvtab[j];
            wtab[i] = LazyReduce1(LazyMulModPrecon(wtab[i-1], w, q, wqinv), q);

            i -= 2; j --;

            for (; i >= 0; i -= 2, j --) {
               long wp2 = wtab[i+2];
               long wm1 = wtab[j];
               wqinvtab[i+2] = LazyPrepMulModPrecon(wp2, q, qinv);
               wtab[i-1] = wm1;
               wqinvtab[i-1] = wqinvtab[j];
               wtab[i] = LazyReduce1(LazyMulModPrecon(wm1, w, q, wqinv), q);
            }

            wqinvtab[1] = LazyPrepMulModPrecon(wtab[1], q, qinv);
         }

         bld.move(item);
      }
   } while (0);
}




#ifdef NTL_BRC_TEST
bool BRC_test_flag = false;
#endif


// FFT: lazy, tables

void NTL_FFT_ROUTINE_TAB(long* A, const long* a, long k, const FFTPrimeInfo& info, long dir)

// performs a 2^k-point convolution modulo q

{
   if (!info.bigtab || k > NTL_FFT_BIGTAB_MAXROOT) {
      NTL_FFT_ROUTINE_NOTAB(A, a, k, info, dir);
      return;
   }

   long q = info.q;
   const long *root = info.RootTable[dir].elts();
   mulmod_t qinv = info.qinv;
   const FFTMultipliers& tab = info.bigtab->MulTab[dir];

   if (k <= 1) {
      if (k == 0) {
	 A[0] = a[0];
	 return;
      }
      if (k == 1) {
	 long a0 = AddMod(a[0], a[1], q);
	 long a1 = SubMod(a[0], a[1], q);
         A[0] = a0;
         A[1] = a1;
	 return;
      }
   }

   // assume k > 1

   if (k >= tab.length()) LazyPrecompFFTMultipliers(k, q, qinv, root, tab);

   NTL_TLS_LOCAL(Vec<unsigned long>, AA_store);
   AA_store.SetLength(1L << k);
   unsigned long *AA = AA_store.elts();


   long n = 1L << k;

#ifndef NTL_BRC_TEST
   BitReverseCopy(AA, a, k);
#else
   if (BRC_test_flag) 
      for (long i = 0; i < n; i++) AA[i] = a[i];
   else
      BitReverseCopy(AA, a, k);
#endif



   /* we work with redundant representations, in the range [0, 4q) */



   long s, m, m_half, m_fourth, i, j; 
   unsigned long t, u, t1, u1;


   // s = 1
   for (i = 0; i < n; i += 2) {
      t = AA[i + 1];
      u = AA[i];
      AA[i] = u + t;
      AA[i+1] = u - t + q;
   }


   // s = 2
   {
      const long * NTL_RESTRICT wtab = tab[2]->wtab_precomp.elts();
      const mulmod_precon_t * NTL_RESTRICT wqinvtab = tab[2]->wqinvtab_precomp.elts();

      const long w1 = wtab[1];
      const mulmod_precon_t wqi1 = wqinvtab[1];

      for (i = 0; i < n; i += 4) {

         unsigned long * NTL_RESTRICT AA0 = &AA[i];
         unsigned long * NTL_RESTRICT AA1 = &AA[i + 2];

         {
            const unsigned long a11 = AA1[0];
            const unsigned long a01 = AA0[0];

            const unsigned long tt1 = a11;
            const unsigned long uu1 = a01;
            const unsigned long b01 = uu1 + tt1; 
            const unsigned long b11 = uu1 - tt1 + 2*q;

            AA0[0] = b01;
            AA1[0] = b11;
         }
         {
            const unsigned long a11 = AA1[1];
            const unsigned long a01 = AA0[1];

            const unsigned long tt1 = LazyMulModPrecon(a11, w1, q, wqi1);
            const unsigned long uu1 = a01;
            const unsigned long b01 = uu1 + tt1; 
            const unsigned long b11 = uu1 - tt1 + 2*q;

            AA0[1] = b01;
            AA1[1] = b11;
         }
      }
   }


   //  s = 3..k

   for (s = 3; s <= k; s++) {
      m = 1L << s;
      m_half = 1L << (s-1);
      m_fourth = 1L << (s-2);

      const long* NTL_RESTRICT wtab = tab[s]->wtab_precomp.elts();
      const mulmod_precon_t * NTL_RESTRICT wqinvtab = tab[s]->wqinvtab_precomp.elts();

      for (i = 0; i < n; i += m) {

         unsigned long * NTL_RESTRICT AA0 = &AA[i];
         unsigned long * NTL_RESTRICT AA1 = &AA[i + m_half];

#if 1

         // a little loop unrolling: this gives the best code

         for (j = 0; j < m_half; j += 4) {
            {
               const long w1 = wtab[j+0];
               const mulmod_precon_t wqi1 = wqinvtab[j+0];
               const unsigned long a11 = AA1[j+0];
               const unsigned long a01 = AA0[j+0];

               const unsigned long tt1 = LazyMulModPrecon(a11, w1, q, wqi1);
               const unsigned long uu1 = LazyReduce2(a01, q);
               const unsigned long b01 = uu1 + tt1; 
               const unsigned long b11 = uu1 - tt1 + 2*q;

               AA0[j+0] = b01;
               AA1[j+0] = b11;
            }
            {
               const long w1 = wtab[j+1];
               const mulmod_precon_t wqi1 = wqinvtab[j+1];
               const unsigned long a11 = AA1[j+1];
               const unsigned long a01 = AA0[j+1];

               const unsigned long tt1 = LazyMulModPrecon(a11, w1, q, wqi1);
               const unsigned long uu1 = LazyReduce2(a01, q);
               const unsigned long b01 = uu1 + tt1; 
               const unsigned long b11 = uu1 - tt1 + 2*q;

               AA0[j+1] = b01;
               AA1[j+1] = b11;
            }
            {
               const long w1 = wtab[j+2];
               const mulmod_precon_t wqi1 = wqinvtab[j+2];
               const unsigned long a11 = AA1[j+2];
               const unsigned long a01 = AA0[j+2];

               const unsigned long tt1 = LazyMulModPrecon(a11, w1, q, wqi1);
               const unsigned long uu1 = LazyReduce2(a01, q);
               const unsigned long b01 = uu1 + tt1; 
               const unsigned long b11 = uu1 - tt1 + 2*q;

               AA0[j+2] = b01;
               AA1[j+2] = b11;
            }
            {
               const long w1 = wtab[j+3];
               const mulmod_precon_t wqi1 = wqinvtab[j+3];
               const unsigned long a11 = AA1[j+3];
               const unsigned long a01 = AA0[j+3];

               const unsigned long tt1 = LazyMulModPrecon(a11, w1, q, wqi1);
               const unsigned long uu1 = LazyReduce2(a01, q);
               const unsigned long b01 = uu1 + tt1; 
               const unsigned long b11 = uu1 - tt1 + 2*q;

               AA0[j+3] = b01;
               AA1[j+3] = b11;
            }
         }

#else

         // a plain loop: not as good as the unrolled version

         for (j = 0; j < m_half; j++) {
            const long w1 = wtab[j];
            const mulmod_precon_t wqi1 = wqinvtab[j];
            const unsigned long a11 = AA1[j];
            const unsigned long a01 = AA0[j];

            const unsigned long tt1 = LazyMulModPrecon(a11, w1, q, wqi1);
            const unsigned long uu1 = LazyReduce2(a01, q);
            const unsigned long b01 = uu1 + tt1; 
            const unsigned long b11 = uu1 - tt1 + 2*q;

            AA0[j] = b01;
            AA1[j] = b11;
         }

#endif

      }
   }

   /* need to reduce redundant representations */

   for (i = 0; i < n; i++) {
      unsigned long tmp = LazyReduce2(AA[i], q);
      A[i] = LazyReduce1(tmp, q);
   }
}





#endif





NTL_END_IMPL

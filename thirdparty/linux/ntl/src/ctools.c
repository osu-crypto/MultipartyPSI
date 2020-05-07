
#include <NTL/ctools.h>


#include <cstdlib>
#include <cmath>

using namespace std;


/*
 * An IEEE double x is finite if and only if x - x == 0.
 * The function _ntl_IsFinite implements this logic;  however,
 * it does not completely trust that an optimizing compiler
 * really implements this correctly, and so it goes out of its way to
 * confuse the compiler.  For a good compiler that respects IEEE floating
 * point arithmetic, this may not be necessary, but it is better
 * to be a bit paranoid.
 *
 * Like the routine _ntl_ForceToMem below, this routine has the
 * side effect of forcing its argument into memory.
 */

NTL_CHEAP_THREAD_LOCAL volatile double _ntl_IsFinite__local = 0;

long _ntl_IsFinite(double *p)
{
   _ntl_IsFinite__local = *p;
   double x1 = _ntl_IsFinite__local;
   double x2 = _ntl_IsFinite__local;
   double x3 = x1-x2;
   if (x3 != 0.0) return 0;
   return 1;
}


/*
 * On machines with wide floating point registers, the routine _ntl_ForceToMem
 * is used to force a floating point double to a memory location.
 * This relies on "separate compilation" model, so that optimizing
 * compilers cannot "optimize away" the whole thing.
 */


#if (NTL_EXT_DOUBLE)

void _ntl_ForceToMem(double *p)
{
   _ntl_IsFinite__local = *p;
   *p = _ntl_IsFinite__local;
}


#else

void _ntl_ForceToMem(double *p)
{ }

#endif



/*
 * The routine _ntl_ldexp(x, e) is like the standard ldexp(x, e) routine,
 * except that it takes a long exponent e, rather than an int exponenet.
 * Some care is taken to ensure reasonable overflow/undeflow behavior.
 * If the value of e does not fit into an int, then the result
 * is x*infinity or x*0, as appropriate.
 * Of course, this can only happen on platforms where long is wider
 * than int (e.g., most 64-bit platforms).
 *
 * We go out of our way to hide the fact that we are multiplying/dividing
 * by zero, so as to avoid unnecessary warnings, and to prevent 
 * overly-agressive optimizing compilers from screwing things up.
 */

NTL_CHEAP_THREAD_LOCAL volatile double _ntl_ldexp_zero = 0.0;

double _ntl_ldexp(double x, long e)
{
   if (e > NTL_MAX_INT)
      return x/_ntl_ldexp_zero;
   else if (e < NTL_MIN_INT)
      return x*_ntl_ldexp_zero;
   else
      return ldexp(x, ((int) e));
}






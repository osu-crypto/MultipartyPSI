#include <NTL/config.h>

#include <ctime>
#include <NTL/ctools.h>

using namespace std;

// FIXME: this is the GetTime that ends up getting used
// on Windows. However, it returns the wall time, not CPU time.
// We could perhaps switch to using GetProcessTimes.
// See: http://nadeausoftware.com/articles/2012/03/c_c_tip_how_measure_cpu_time_benchmarking

// NOTE: in this version, because clock_t can overflow fairly
// quickly (in less than an hour on some systems), we provide
// a partial work-around, by tracking the differences between calls

double _ntl_GetTime()
{
   static NTL_CHEAP_THREAD_LOCAL clock_t last_clock = 0;
   static NTL_CHEAP_THREAD_LOCAL double acc = 0;

   clock_t this_clock;
   double delta;

   this_clock = clock();

   delta = (this_clock - last_clock)/((double)CLOCKS_PER_SEC);
   if (delta < 0) delta = 0;

   acc += delta;
   last_clock = this_clock;

   return acc;
}




#include <NTL/GF2E.h>

#include <NTL/new.h>

NTL_START_IMPL

NTL_TLS_GLOBAL_DECL(SmartPtr<GF2EInfoT>, GF2EInfo_stg)

NTL_CHEAP_THREAD_LOCAL
GF2EInfoT *GF2EInfo = 0; 


GF2EInfoT::GF2EInfoT(const GF2X& NewP)
{
   build(p, NewP);

   if (p.size == 1) {
      if (deg(p) <= NTL_BITS_PER_LONG/2)
         KarCross = 4;
      else
         KarCross = 8;
   }
   else if (p.size == 2)
      KarCross = 8;
   else if (p.size <= 5)
      KarCross = 4;
   else if (p.size == 6)
      KarCross = 3;
   else 
      KarCross = 2;


   if (p.size <= 1) {
      if (deg(p) <= NTL_BITS_PER_LONG/2)
         ModCross = 20;
      else
         ModCross = 40;
   }
   else if (p.size <= 2)
      ModCross = 75;
   else if (p.size <= 4)
      ModCross = 50;
   else
      ModCross = 25;

   if (p.size == 1) {
      if (deg(p) <= NTL_BITS_PER_LONG/2)
         DivCross = 100;
      else
         DivCross = 200;
   }
   else if (p.size == 2)
      DivCross = 400;
   else if (p.size <= 4)
      DivCross = 200;
   else if (p.size == 5)
      DivCross = 150;
   else if (p.size <= 13)
      DivCross = 100;
   else 
      DivCross = 75;

   _card_exp = p.n;
}


const ZZ& GF2E::cardinality()
{
   if (!GF2EInfo) LogicError("GF2E::cardinality: undefined modulus");

   do { // NOTE: thread safe lazy init
      Lazy<ZZ>::Builder builder(GF2EInfo->_card);
      if (!builder()) break;
      UniquePtr<ZZ> p;
      p.make();
      power(*p, 2, GF2EInfo->_card_exp);
      builder.move(p);
   } while (0);

   return *GF2EInfo->_card;
}







void GF2E::init(const GF2X& p)
{
   GF2EContext c(p);
   c.restore();
}


void GF2EContext::save()
{
   NTL_TLS_GLOBAL_ACCESS(GF2EInfo_stg);
   ptr = GF2EInfo_stg;
}

void GF2EContext::restore() const
{
   NTL_TLS_GLOBAL_ACCESS(GF2EInfo_stg);
   GF2EInfo_stg = ptr;
   GF2EInfo = GF2EInfo_stg.get();
}



GF2EBak::~GF2EBak()
{
   if (MustRestore) c.restore();
}

void GF2EBak::save()
{
   c.save();
   MustRestore = true;
}


void GF2EBak::restore()
{
   c.restore();
   MustRestore = false;
}



const GF2E& GF2E::zero()
{
   static const GF2E z(INIT_NO_ALLOC); // GLOBAL (assumes C++11 thread-safe init)
   return z;
}



istream& operator>>(istream& s, GF2E& x)
{
   GF2X y;

   NTL_INPUT_CHECK_RET(s, s >> y);
   conv(x, y);

   return s;
}

void div(GF2E& x, const GF2E& a, const GF2E& b)
{
   GF2E t;

   inv(t, b);
   mul(x, a, t);
}

void div(GF2E& x, GF2 a, const GF2E& b)
{
   inv(x, b);
   mul(x, x, a);
}

void div(GF2E& x, long a, const GF2E& b)
{
   inv(x, b);
   mul(x, x, a);
}


void inv(GF2E& x, const GF2E& a)
{
   InvMod(x._GF2E__rep, a._GF2E__rep, GF2E::modulus());
}

NTL_END_IMPL

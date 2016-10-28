#include "BitIterator.h"

namespace bOPRF
{

	BitReference::operator u8() const
	{
		return (*mByte & mMask) >> mShift;
	}

}
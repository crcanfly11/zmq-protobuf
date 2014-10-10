#include "yt/util/hashfun.h"
#include <math.h>

namespace yt
{
	unsigned long hashpjw(char *arKey, unsigned int nKeyLength)
	{
		unsigned long h = 0, g;
		char *arEnd=arKey+nKeyLength;

		while (arKey < arEnd) {
			h = (h << 4) + *arKey++;
			if ((g = (h & 0xF0000000))) {
				h = h ^ (g >> 24);
				h = h ^ g;
			}
		}
		return h;
	}	
	size_t GetFitSize(size_t size,size_t minblocksize,double exp,size_t& power)
	{
		if(size <= minblocksize)
		{
			power = 0;
			return minblocksize;
		}

		double d = log((double)size/(double)minblocksize)/log(exp);

		if((double)((int)d) == d)
		{
			power = (size_t)d;
			return (size_t)(minblocksize * pow(exp,d));
		}

		power = (size_t)(d+1);
		return (size_t)(minblocksize * pow(exp,(int)(d+1)));
	}

}

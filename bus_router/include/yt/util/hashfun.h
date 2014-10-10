#pragma once

#include <sys/types.h>

namespace yt
{
	unsigned long hashpjw(char *arKey, unsigned int nKeyLength);
	size_t GetFitSize(size_t size,size_t minblocksize,double exp,size_t& power);
}


#include "yt/log/nulllogger.h"

namespace yt
{

	NullLogger * NullLogger::Instance()
	{
		static NullLogger oNullLogger;
		return &oNullLogger;
	}

}


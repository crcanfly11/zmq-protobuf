#pragma once

namespace yt
{
	inline long time_use(timeval *timebegin,timeval *timeend)
	{
		long timeuse = 1000000*(timeend->tv_sec-timebegin->tv_sec)+ timeend->tv_usec-timebegin->tv_usec;
		return timeuse;
	}
}


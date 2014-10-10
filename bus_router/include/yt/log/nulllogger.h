#pragma once

#include <string>
#include "yt/log/logger.h"

namespace yt
{
	class NullLogger : public Logger
	{
		public:
			virtual int Log(LogPriority, const std::string &)
			{ return 0; }

		public:
			static NullLogger * Instance();
	};

}


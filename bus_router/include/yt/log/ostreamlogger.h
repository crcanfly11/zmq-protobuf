#pragma once

#include <string>
#include <iostream>
#include "yt/util/mutex.h"
#include "yt/log/logger.h"

namespace yt
{
	class OStreamLogger : public Logger
	{
		public:
			OStreamLogger(std::ostream & stream, const std::string & sLogFormat=/*Logger::*/DEFAULT_FORMAT);

			virtual int Log(LogPriority , const std::string & s);

		public:
			//static OStreamLogger StdoutLogger;
			//static OStreamLogger StderrLogger;

		private:
			std::ostream & stream_;
			ThreadMutex mutex_;
	};
}


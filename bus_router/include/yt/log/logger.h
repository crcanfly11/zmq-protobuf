#pragma once

#include <string>
#include "yt/log/logpriority.h"

namespace yt
{
	#define DEFAULT_FORMAT "[%D][%P][%t] %m (%F:%L %f)"
	class Logger
	{
		public:
			Logger(const std::string &sLogFormat = DEFAULT_FORMAT) : sLogFormat_(sLogFormat) {}
			virtual ~Logger() {}
			virtual int Log(LogPriority, const std::string &) = 0;
			inline const std::string & GetFormat() const{return sLogFormat_;}
		private:
			const std::string sLogFormat_;
	};
}


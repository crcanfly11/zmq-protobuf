#pragma once

#include <string>
#include <arpa/inet.h>

#include "yt/log/logger.h"

namespace yt
{
	class UdpLogger : public Logger
	{
		public:
			UdpLogger(const char * sIP, unsigned short uPort, const std::string & sLogFormat=/*Logger::*/DEFAULT_FORMAT);
			virtual int Log(LogPriority, const std::string & s);

		private:
			struct sockaddr_in servaddr_;
	};
}


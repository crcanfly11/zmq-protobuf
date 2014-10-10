#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include "yt/log/udplogger.h"

namespace yt
{
	UdpLogger::UdpLogger(const char * sIP, unsigned short uPort, const std::string & sLogFormat)
		: Logger(sLogFormat)
		{
			memset(&servaddr_, 0, sizeof(servaddr_));
			servaddr_.sin_family = AF_INET;
			servaddr_.sin_port = htons(uPort);        
			inet_pton(AF_INET, sIP, &servaddr_.sin_addr);	
		}

	int UdpLogger::Log(LogPriority, const std::string & s)
	{
		int sockfd = socket(SOCK_DGRAM, AF_INET, 0);
		if ( sockfd < 0 ) {
			return -1;
		}

		ssize_t len = sendto(sockfd, s.c_str(), s.length(), 0, (struct sockaddr *)&servaddr_, sizeof(servaddr_));
		if ( len < 0 ) {
			close(sockfd);
			return -1;
		}
		close(sockfd);

		return 0;
	}
}


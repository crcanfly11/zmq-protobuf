#include <time.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include "yt/log/datefilelogger.h"
#include "yt/util/mutex.h"
#include "yt/util/scope_guard.h"
#include "yt/util/lockguard.h"

using namespace std;

namespace yt
{
	DateFileLogger::DateFileLogger(const std::string & sLogFile, const std::string & sLogFormat)
		: Logger(sLogFormat), sLogFile_(sLogFile),m_fd(-1)
	{
		time_t tTime = time(0);
		localtime_r(&tTime, &cur_time);
	}
	DateFileLogger::~DateFileLogger()
	{
		if(m_fd != -1)
			close(m_fd);
	}

	string DateFileLogger::GetLogFileName()
	{
		time_t tTime = time(0);

		struct tm stTime;
		localtime_r(&tTime, &stTime);

		char szFormatTime[32];
		strftime(szFormatTime, sizeof(szFormatTime)-1, ".%Y-%m-%d", &stTime);

		return sLogFile_ + szFormatTime + ".log";
	}

	int DateFileLogger::Log(LogPriority ,const string & sLogMsg)
	{
	//	mutex_.Acquire();
	//	LockGuard g(&mutex_);
                
		if(m_fd < 0)
		{
			mutex_.Acquire();
			if(m_fd < 0)
			{
				m_fd = open(GetLogFileName().c_str(), O_CREAT | O_WRONLY | O_APPEND | O_LARGEFILE, 0644);
				if(m_fd < 0) 
				{
					mutex_.Release();
					return -1;
				}
			}
			mutex_.Release();
		}
		struct tm now;
		time_t tTime = time(0);
		localtime_r(&tTime, &now);
		if(cur_time.tm_mday != now.tm_mday)
		{
			mutex_.Acquire();
			if(cur_time.tm_mday != now.tm_mday)
			{
				cur_time = now;
				int fd = open(GetLogFileName().c_str(), O_CREAT | O_WRONLY | O_APPEND | O_LARGEFILE, 0644);
				close(m_fd);
				m_fd = fd;
			}
			mutex_.Release();

		}
		if ( pwrite(m_fd, sLogMsg.c_str(), sLogMsg.size(),SEEK_END)!=(int)sLogMsg.size() /*|| write(fd, "\n", 1) != 1*/ ) 
		{
			close(m_fd);
			m_fd = -1;
			return -1;
		}
		return 0;
	}
}


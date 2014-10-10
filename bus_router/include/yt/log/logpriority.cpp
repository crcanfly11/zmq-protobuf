#include "yt/log/logpriority.h"
#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <sstream>
#include <pthread.h>
#include <unistd.h>
#include <iomanip>
#include <sys/time.h>

#include <iostream>
using namespace std;

namespace yt
{
	const char * __LPDesc[] =
	{
		"BASE",

		"AC_TRACE",
		"AC_DEBUG",
		"AC_INFO",
		"AC_WARNING",
		"AC_ERROR",
		"AC_CRITICAL",

		"TRACE",
		"DEBUG",
		"INFO",
		"USER1",
		"USER2",
		"WARNING",
		"ERROR",
		"CRITICAL"
	};

	static const char * GetLogPriorityDesc(LogPriority lp)
	{
		int i = 0;
		int array_size = sizeof(__LPDesc) / sizeof(__LPDesc[0]);

		for ( ; i<array_size; ++i ) {
			if ( lp >> i & 1 ) {
				break;
			}
		}

		if ( i == array_size ) {
			return "UNKNOWN";
		}
		else {
			return __LPDesc[i];
		}
	}

	static int FormatDate(string &s)
	{
		struct timeval tv;
		gettimeofday(&tv, NULL);

		struct tm stTime;
		localtime_r(&tv.tv_sec, &stTime);

		char szFormatTime[128];
		memset(szFormatTime, 0x0, sizeof(szFormatTime));	
		strftime(szFormatTime, sizeof(szFormatTime)-1, "%Y-%m-%d %H:%M:%S", &stTime);

		ostringstream os;
		os << szFormatTime << " " << setw(6) << setfill('0') << tv.tv_usec;

		s.append(os.str());
		return 0;
	}

	static int FormatFile(const char * File, string &s)
	{
		string sFile(File);
		size_t len = sFile.size();
		size_t pos = sFile.find_last_of('/',string::npos);
		if(pos != string::npos)
			s.append(sFile.substr(pos + 1,len - pos - 1));
		else
			s.append(File);
		return 0;
	}


	static int FormatLine(int iLine, string &s)
	{
		ostringstream os;
		os << iLine;

		s.append(os.str());
		return 0;
	}

	static int FormatThreadID(string &s)
	{
		ostringstream os;
		os << pthread_self();

		s.append(os.str());
		return 0;
	}

	static int FormatProcessID(string &s)
	{
		ostringstream os;
		os << getpid();

		s.append(os.str());
		return 0;
	}

	static int FormatPriority(LogPriority iLogPriority, string &s)
	{
		s.append(GetLogPriorityDesc(iLogPriority));
		return 0;
	}

	std::string FormatLog(const std::string &sFormat,LogPriority iLogPriority,const char *szFile,int iLine,const char *szFunction, const char *szFmt, ...)
	{
		std::string s;
		s.reserve(64);

		for(size_t i = 0;i < sFormat.size();++i)
		{
			if ( sFormat[i] == '%' ) 
			{
				++i;
				switch ( sFormat[i] )
				{
					case 'D': FormatDate(s); break;
					case 'P': FormatPriority(iLogPriority, s); break;
					case 'F': FormatFile(szFile, s); break;
					case 'L': FormatLine(iLine, s); break;
					case 't': FormatThreadID(s); break;
					case 'p': FormatProcessID(s); break;
					case 'f': s.append(szFunction); break;
					case 'm': 
						  {
							  enum { LOGMSG_MAX_LENGTH = 8192 };

							  char szLogMsg[LOGMSG_MAX_LENGTH];
							  //memset(szLogMsg, 0x0, sizeof(szLogMsg));

							  va_list ap;
							  va_start(ap, szFmt);
							  vsnprintf(szLogMsg, LOGMSG_MAX_LENGTH, szFmt, ap);
							  va_end(ap);

							  s.append(szLogMsg);
							  break;
						  }	
					default: --i; s.append(1, '%'); break;
				}
			}
			else
			{
				s.append(1, sFormat[i]);
			}
		}
		s.append("\n");

		return s;
	}
}


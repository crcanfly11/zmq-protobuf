#include <string>
#include <sstream>
#include <iomanip>
#include "yt/log/logger.h"
#include <time.h>
#include <sys/time.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>

using namespace std;

extern bool g_stdlogger;
// StdLogger是否生效,false不打印，true打印
inline void StdLoggerEnabled(bool enabled){ g_stdlogger = enabled;}

namespace yt
{
	class StdLogger:public Logger
	{
	public:
		StdLogger(const string& fileName = "StdLogger");
		~StdLogger();
		// 时间戳
		//void SetTimestamp(const string& t);
		// 源IP地址
		void SetSrcIP(const string& ip);
		// 目标地址 IP:PORT
		void SetDestAddr(const string& addr);
		// 引用页
		void SetReferer(const string& ref);
		// 设置请求ID
		void SetRequestID(string reqid);
		// 设置请求串
		void SetRequestString(const string& reqstr);
		// 设置服务端返回码
		void SetResponseCode(const string& rcode);
		// 设置处理消耗的时间
		void SetTimespent(const long t = 0);
		// 设置返回字节数
		void SetReturnBytes(const long len = 0);
		// 设置返回数据类型
		void SetReturnDataType(const string& type); 
		// 写日志
		void Log();
		
	private:
		string GetLogFileName();
		int Log(LogPriority lp, const std::string& msg);
		void FormatLog(string& log);
		
		void FormatDate(string &s)
		{
			struct timeval tv; 
			gettimeofday(&tv, NULL);

			struct tm stTime;
			localtime_r(&tv.tv_sec, &stTime);

			char szFormatTime[128];
			sprintf(szFormatTime,"%04d-%02d-%02d %02d:%02d:%02d %03d",stTime.tm_year + 1900,stTime.tm_mon + 1,stTime.tm_mday,stTime.tm_hour,stTime.tm_min,stTime.tm_sec,(int)tv.tv_usec / 1000);
			s.append(szFormatTime);
	
		}
		
		
	private:
		string m_timestamp;
		string m_srcip;
		string m_destaddr;
		string m_referer;
		string m_reqid;
		string m_reqstring;
		string m_responsecode;
		long m_timespent;
		long m_retbytes;
		string m_rettype;
		
		string m_logFileName;
		
		//bool m_written;
	};
};


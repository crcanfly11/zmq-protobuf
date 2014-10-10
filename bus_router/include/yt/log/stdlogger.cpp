#include <unistd.h>
#include "stdlogger.h"
#include "yt/util/scope_guard.h"

using namespace std;

bool g_stdlogger = false;

namespace yt
{
	StdLogger::StdLogger(const string& fileName):
		Logger(),
		m_logFileName(fileName)
	{
		
	}

	StdLogger::~StdLogger()
	{
		if(g_stdlogger)
		{
			Log();
		}
	}

	// 时间戳
	// void StdLogger::SetTimestamp(const string& t)
	// {
		// m_timestamp = t;
	// }

	// 源IP地址
	void StdLogger::SetSrcIP(const string& ip)
	{
		m_srcip = ip;
	}

	// 目标地址 IP:PORT
	void StdLogger::SetDestAddr(const string& addr)
	{
		m_destaddr = addr;
	}

	// 引用页
	void StdLogger::SetReferer(const string& ref)
	{
		m_referer = ref;
	}

	// 设置请求ID
	void StdLogger::SetRequestID(string reqid)
	{
		m_reqid = reqid;
	}

	// 设置请求串
	void StdLogger::SetRequestString(const string& reqstr)
	{
		m_reqstring = reqstr;
	}

	// 设置服务端返回码
	void StdLogger::SetResponseCode(const string& rcode)
	{
		m_responsecode = rcode;
	}

	// 设置处理消耗的时间
	void StdLogger::SetTimespent(const long t)
	{
		m_timespent = t;
	}

	// 设置返回字节数
	void StdLogger::SetReturnBytes(const long len)
	{
		m_retbytes = len;
	}

	// 设置返回数据类型
	void StdLogger::SetReturnDataType(const string& type)
	{
		m_rettype = type;
	}
	
	string StdLogger::GetLogFileName()
	{
		time_t tTime = time(0);

		struct tm stTime;
		localtime_r(&tTime, &stTime);

		char szFormatTime[32];
		strftime(szFormatTime, sizeof(szFormatTime)-1, ".%Y-%m-%d", &stTime);

		return m_logFileName + szFormatTime + ".log";
	}
	
	void StdLogger::FormatLog(string& log)
	{
		log.clear();
		
		string strTime;
		FormatDate(strTime);
		
		log.append("\"");
		log.append(strTime);
		log.append("\" \"");
		log.append(m_srcip);
		log.append("\" \"");
		log.append(m_destaddr);
		log.append("\" \"");
		log.append(m_referer);
		log.append("\" \"");
				
		log.append(m_reqid);
		
		log.append("\" \"");
		log.append(m_reqstring);
		log.append("\" \"");
		log.append(m_responsecode);
		log.append("\" \"");
		
		char buf[50];
		sprintf(buf, "%ld", m_timespent);
		log.append(buf);
		
		log.append("\" \"");
		
		sprintf(buf, "%ld", m_retbytes);				
		log.append(buf);
		
		log.append("\" \"");
		log.append(m_rettype);
		log.append("\"\n");
	
		// char buf[512];
		// memset(buf, 0, sizeof(buf));
		// /*
		// timestamp srcip dstip:port 'referer' reqid reqstring responsecode timespent return_bytes return_datatype 
		// 时间戳(到毫秒) 源IP 目标IP和端口 引用页(加引号) 请求ID 请求串(带参数) 服务端返回码 服务端处理消耗的时间 返回字节数 返回数据类型 
		// */
		
		// string strTime;
		// FormatDate(strTime);
		
		// sprintf(buf, "\"%s\" \"%s\" \"%s\" \"%s\" \"%d\" \"%s\" \"%s\" \"%ld\" \"%ld\" \"%s\" \n", 
				// strTime.c_str(), m_srcip.c_str(), 
				// m_destaddr.c_str(), m_referer.c_str(), 
				// m_reqid, m_reqstring.c_str(), 
				// m_responsecode.c_str(), m_timespent, 
				// m_retbytes, m_rettype.c_str()
				// );
				
		// log.assign(buf, strlen(buf));
	}
	
	void StdLogger::Log()
	{
		string log;
		FormatLog(log);
		Log(LP_STANDARD, log);
	}
	
	int StdLogger::Log(LogPriority lp, const string& msg)
	{
		int fd = open(GetLogFileName().c_str(), O_CREAT | O_WRONLY | O_APPEND | O_LARGEFILE, 0644);
		if(fd < 0)
			return -1;
			
		AC_ON_BLOCK_EXIT(close, fd);
		
		if ( pwrite(fd, msg.c_str(), msg.size(),SEEK_END)!=(int)msg.size()) 			
			return -1;	
		
		return 0;
	}
}


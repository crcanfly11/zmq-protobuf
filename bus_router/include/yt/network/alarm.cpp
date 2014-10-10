#include "alarm.h"
#include "yt/log/log.h"
//#include "yt/util/threadbuf.h"

namespace yt
{
	Alarm *g_alarm = NULL;
	void Alarm::Notify(int level,short type,const char *info)
	{
		AlarmInfo alarminfo;
		alarminfo.level = level;
		alarminfo.type = type;
		//strcpy(alarminfo.info,info);
		alarminfo.info.append(info);
		
		m_alarmqueue.Push(alarminfo);

		send(m_fd[0],"A",1,0);
	}
	void Alarm::Notify(const string & info)
	{
		m_infoqueue.Push(info);

		send(m_fd[0],"A",1,0);
	}
	bool Alarm::Init()
	{
		if(socketpair( AF_UNIX, SOCK_STREAM, 0, m_fd ) < 0)
		{
			AC_ERROR("socketpair error");
			return false;
		}
		if(FDEventHandler::SetNonBlocking(m_fd[0],true) != 0)
		{
			AC_ERROR("set non blocing error");
			return false;
		}
		if(FDEventHandler::SetNonBlocking(m_fd[1],true) != 0)
		{
			AC_ERROR("set non blocing error");
			return false;
		}
		m_handler->SetFD(m_fd[1]);
		if(m_handler->RegisterRead((struct timeval*)NULL) != 0)
		{
			AC_ERROR("register socket pair error");
			return false;
		}
		return true;
	}
	void Alarm1(short type,const char *info)
	{
		if(g_alarm)
			g_alarm->Alarm1(type,info);
	}
	void Alarm2(short type,const char *info)
	{
		if(g_alarm)
			g_alarm->Alarm2(type,info);
	}
	void Alarm3(short type,const char *info)
	{
		if(g_alarm)
			g_alarm->Alarm3(type,info);
	}
	void Alarm4(short type,const char *info)
	{
		if(g_alarm)
			g_alarm->Alarm4(type,info);
	}
	void InfoPush(const string & info)
	{
		if(g_alarm)
			g_alarm->InfoPush(info);
	}
}


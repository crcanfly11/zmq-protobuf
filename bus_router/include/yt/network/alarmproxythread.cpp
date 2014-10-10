#include "yt/network/alarmproxythread.h"
//#include "yt/network/netproxy.cpp"
#include "alarm.h"

namespace yt
{
	AlarmProxyThread::AlarmProxyThread(const char *host,int port,int maxclientcount,int conntimeout,int maxinpacklen,int maxoutpacklen,DataDecoderBase *clientdatadecoder) : NetProxy<>(host,port,maxclientcount,conntimeout,maxinpacklen,maxoutpacklen,clientdatadecoder) , m_alarmeventhandler(NULL){}

	AlarmProxyThread::AlarmProxyThread(int fd,int maxclientcount,int conntimeout,int maxinpacklen,int maxoutpacklen,DataDecoderBase *clientdatadecoder) : NetProxy<>(fd,maxclientcount,conntimeout,maxinpacklen,maxoutpacklen,clientdatadecoder) , m_alarmeventhandler(NULL){}

	AlarmProxyThread::~AlarmProxyThread()
	{
		if(m_alarmeventhandler)
			delete m_alarmeventhandler;
		if(g_alarm)
			delete g_alarm;
	}

	bool AlarmProxyThread::Init()
	{
		if(!NetProxy<>::Init())
			return false;

		m_alarmeventhandler = new(nothrow) AlarmEventHandler(GetReactor(),GetClientMap());
		if(!m_alarmeventhandler)
			return false;

		g_alarm = new(nothrow) Alarm(m_alarmeventhandler);
		if(!g_alarm)
			return false;

		if(!g_alarm->Init())
			return false;
		
		return true;
	}
	void AlarmProxyThread::UnInit()
	{
		m_alarmeventhandler->Close();
		NetProxy<>::UnInit();
	}
	void AlarmProxyThread::Run()
	{
		if(!NetProxy<>::Start())
			return;
		UnInit();
	}
}

#pragma once

#include "yt/network/netproxy.h"
#include "yt/network/alarmeventhandler.h"
#include "yt/util/thread.h"

namespace yt
{
	//报警线程类
	class AlarmProxyThread : public NetProxy<> , public Thread
	{
		public:
			AlarmProxyThread(const char *host,int port,int maxclientcount,int conntimeout,int maxinpacklen,int maxoutpacklen,DataDecoderBase *clientdatadecoder);
			AlarmProxyThread(int fd,int maxclientcount,int conntimeout,int maxinpacklen,int maxoutpacklen,DataDecoderBase *clientdatadecoder);
			inline int StartAlarmThread(){return Thread::Start();}
			virtual ~AlarmProxyThread();
			bool Init();
			void UnInit();
			virtual void Run();
		private:
			AlarmEventHandler *m_alarmeventhandler;
	};
}

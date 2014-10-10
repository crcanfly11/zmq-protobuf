#pragma once

#include "yt/network/listensocket.h"
#include "yt/network/clientsocket.h"
#include "yt/network/reactor.h"
#include "yt/network/dataprocessor.h"
#include "yt/network/datadecoder.h"
#include "yt/util/notify.h"
#include "yt/util/mutex.h"
#include "yt/util/tsvector.h"
#include "yt/util/thread.h"

namespace yt
{
	class ClientThread : public Thread
	{
		friend class NetApp2;
		public:
			ClientThread(int fd,size_t maxoneinpacklen,int timeout,DataProcessor *processor,TSList<ClientThread*> *threadlist,TSVector<long> *timelist);
			~ClientThread();

			void Run();
			inline void Stop(){m_flag = false;}
		private:
			int m_fd;
			size_t m_maxoneinpacklen;
			int m_timeout;
			DataProcessor *m_processor;
			TSList<ClientThread*> *m_threadlist;
			TSVector<long> *m_timelist;

			bool m_flag;
	};
	//领跑者跟随者的实现，现在不用了,用NetProxyQueueThreadsProcess取代了
	class NetApp2
	{
		public:
			NetApp2(const char *host,int port,int maxclient,DataProcessor *processor,int timeout,size_t maxoneinpacklen) : 
				m_host(host),
				m_port(port),
				m_maxclient(maxclient),
				m_processor(processor),
				m_timeout(timeout),
				m_maxoneinpacklen(maxoneinpacklen),
				m_flag(true){}
			virtual ~NetApp2(){}
			bool Start();
			inline void Stop() {m_flag = false;}
			long GetSpeed();
			int GetClientCount();
			void SendAll(char *buf,size_t buflen);
			virtual bool OnAccept(int /*fd*/){return true;}
		private:
			string m_host;
			int m_port;
			int m_maxclient;
			DataProcessor *m_processor;
			int m_timeout;
			size_t m_maxoneinpacklen;

			bool m_flag;

			TSList<ClientThread*> m_threadlist;
			TSVector<long> m_timelist;
	};
}


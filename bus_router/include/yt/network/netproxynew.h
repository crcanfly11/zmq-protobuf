#pragma once

#include <limits.h>
#include "yt/network/reactor.h"
#include "yt/network/listensocket.h"
#include "yt/network/datadecoder.h"
#include "yt/network/clientmap.h"
#include "yt/util/tsvector.h"
#include "yt/util/thread.h"

namespace yt
{
	class NotifyEventHandler2 : public FDEventHandler
	{
		public:
			NotifyEventHandler2(Reactor *pReactor) : FDEventHandler(pReactor) {}

			bool Init();
			void Close();

			virtual void OnFDRead(){ RecvNotify(); }
			virtual void OnFDWrite(){};
			virtual void OnFDReadTimeOut(){};
			virtual void OnFDWriteTimeOut(){};

			int GetSendFD() { return m_fdpair[0]; }
			int GetRecvFD() { return m_fdpair[1]; }

			virtual void SendNotify(const char* buf, size_t len) = 0;
			virtual void RecvNotify() = 0;

		private:
			int m_fdpair[2];					// 0: 发送; 1: 接收
	};
	template <class CLIENTSOCKET2>class NetNewProxy;

	template <class CLIENTSOCKET1 = ClientSocket>
	class FdNotifyEventHandler2 : public NotifyEventHandler2
	{
		public:
			FdNotifyEventHandler2(Reactor *pReactor) : NotifyEventHandler2(pReactor) {}

			void SetProcessHandler(NetNewProxy<CLIENTSOCKET1> *pMyThread) { m_pMyThread = pMyThread; }

			void SendNotify(const char* buf, size_t len)
			{
				// 阻塞发送
				if (send(GetSendFD(), buf, len, 0) != (ssize_t)len)
					AC_ERROR("send fd error.");	
			}
			void SendNotify(int fd)
			{
				SendNotify((char*)(&fd), sizeof(int));
			}
			void RecvNotify()
			{
				int fd; 

				if (recv(FDEventHandler::m_fd, (char*)&fd, sizeof(int), 0) != sizeof(int))
					AC_ERROR("recv fd error.");

				if (!m_pMyThread->ApplyClient(fd))
				{   
					AC_ERROR("apply client error.");
				}   
			}

		private:
			NetNewProxy<CLIENTSOCKET1> *m_pMyThread;
	};

	typedef void (*OnAccept2)(void *);
	template <class CLIENTSOCKET = ClientSocket>
		class NetNewProxy
		{
			public:
				NetNewProxy(int maxclientcount,int conntimeout,int maxinpacklen,int maxoutpacklen,DataDecoderBase *clientdatadecoder):m_maxclient(maxclientcount), m_conntimeout(conntimeout), m_maxinpacklen(maxinpacklen), m_maxoutpacklen(maxoutpacklen),m_listennotify(&m_reactor),m_pDecoder(clientdatadecoder)
			{
				m_listennotify.SetProcessHandler(this);
			}
				virtual ~NetNewProxy(){}
				virtual bool Init()
				{
					if(m_reactor.Init() != 0)
					{
						AC_ERROR("reactor init error");
						return false;
					}

					if (!m_listennotify.Init())
					{
						AC_ERROR("init notify error.");
						return false;
					}

					return true;
				}
				virtual bool Start()
				{
					m_reactor.Run();
					return true;
				}
				virtual void Stop()
				{
					m_reactor.Stop();
				}
				virtual void UnInit()
				{
					m_clientmap.Close();
					m_listennotify.Close();
				}
				inline Reactor* GetReactor()
				{	
					return &m_reactor;
				}
				inline ClientMap* GetClientMap(){
					return &m_clientmap;
				}
				int CalcWeight()
				{
					if (m_clientmap.Size() >= (size_t)m_maxclient)
						return INT_MAX;

					return m_clientmap.Size();
				}
				bool ApplyClient(int fd)
				{
					if((int)m_clientmap.Size() > m_maxclient)
					{   
						AC_ERROR("too many client");
						close(fd);
						return false;
					}   

					// proxy的逻辑处理
					CLIENTSOCKET *pClient = new(std::nothrow) CLIENTSOCKET(&m_reactor, m_pDecoder, &m_clientmap, fd, m_maxinpacklen, m_maxoutpacklen);
					if (!pClient)
					{   
						AC_ERROR("Cann't Allocate client in pool");
						Alarm3(AT_MEMERROR,"cann't allocate client");
						close(fd);
						return false;
					}   

					// 插入clientmap
					InsertClient(pClient);
					//AC_INFO("network thread[%d], count: [%d]\n", m_id, m_clientmap.Size());

					if(m_fun)
						m_fun(pClient);

					return true;
				}
				inline void SetOnAccept2(OnAccept2 fun){
					m_fun = fun;
				}

			private:
				void InsertClient(CLIENTSOCKET *pClient)
				{
					int seq = m_counter.Get();
					while(m_clientmap.Get(seq) != NULL)
					{   
						AC_INFO("seq overlap");
						seq = m_counter.Get();
					}   

					pClient->SetClientID(seq);              // 设置客户端的id
					m_clientmap.Put(seq, pClient);              // 放入clientmap

					if(pClient->RegisterRead(m_conntimeout) != 0)   //注册读事件
					{   
						AC_ERROR("register read error");
						pClient->Close();
						return;
					}  	
				}

			public:
				int m_maxclient;									// 线程处理的最大客户数
				int m_conntimeout;
				int m_maxinpacklen;
				int m_maxoutpacklen;

				FdNotifyEventHandler2<CLIENTSOCKET> m_listennotify;				// 与listen交互的notify
				DataDecoderBase *m_pDecoder;
				EPReactor m_reactor;
				Counter m_counter;
				ClientMap m_clientmap;
				OnAccept2 m_fun;
		};

	template <class CLIENTSOCKET3 = ClientSocket>
	class NbListen2 : public ListenSocketBase
	{
		public:
			NbListen2(int fd, Reactor *pReactor, vector<NetNewProxy<CLIENTSOCKET3> *> *pthreadlist) : 
				ListenSocketBase(fd, pReactor, 0), m_pthreadlist(pthreadlist), m_lastid(0) {}
			NbListen2(const char *host,int port,Reactor *pReactor,vector<NetNewProxy<CLIENTSOCKET3> *> *pthreadlist) :
				ListenSocketBase(host, port, pReactor, 0), m_pthreadlist(pthreadlist), m_lastid(0) {}

			virtual void OnAccept(int fd) 
			{   
				// 选择一个当前最恰当的proxy
				NetNewProxy<CLIENTSOCKET3> *thread = SelectThread();
				if (!thread)
				{   
					// 所有的proxy都满了
					AC_ERROR("all thread full");
					close(fd);
					return;
				}   

				// 向proxy发送通知消息
				thread->m_listennotify.SendNotify(fd);

				return;
			}

		private:
			NetNewProxy<CLIENTSOCKET3> * SelectThread()
			{
				int minweight = INT_MAX;
				NetNewProxy<CLIENTSOCKET3>* thread = NULL;
				size_t size = m_pthreadlist->size();
				int id = m_lastid;

				for (size_t i = m_lastid; i < size + m_lastid; i++)
				{   
					size_t j = i % size;
					int weight = (*m_pthreadlist)[j]->CalcWeight();       // CalcWeight()函数必须非常轻

					if (weight < minweight)
					{   
						minweight = weight;
						thread = (*m_pthreadlist)[j];
						id = (j + size + 1) % size;
					}   
				}   

				m_lastid = id; 

				return thread;
			}

		private:
			vector<NetNewProxy<CLIENTSOCKET3> *> *m_pthreadlist;
			int m_lastid;
	};

	template <class CLIENTSOCKET4 = ClientSocket>
	class NetListenProxy : public Thread
	{
		public:
			NetListenProxy(const char *host,int port,vector<NetNewProxy<CLIENTSOCKET4> *> *pthreadlist):
			m_listen(host, port, &m_reactor, pthreadlist) {}
			NetListenProxy(int fd,vector<NetNewProxy<CLIENTSOCKET4> *> *pthreadlist):
				m_listen(fd, &m_reactor, pthreadlist) {}
			virtual ~NetListenProxy() {}
			inline int StartThread() {return Thread::Start();}
			inline void Run()
			{
				AC_INFO("listenproxy thread start.");
				if (!Init())
				{
					AC_ERROR("init error.");
					return;	    
				}
				if (!Start())
				{
					AC_ERROR("start error");
					return;
				}
				Stop();
				UnInit();
			}
			virtual bool Init()
			{
				if(m_reactor.Init() != 0)
				{
					AC_ERROR("reactor init error");
					return false;
				}

				return true;
			}
			virtual bool Start()
			{
				if(m_listen.Listen() != 0)
				{
					AC_ERROR("main listen error");
					return false;
				}

				m_reactor.Run();

				return true;
			}
			virtual void Stop()
			{
				m_reactor.Stop();
			}
			virtual void UnInit()
			{
				m_listen.Close();
			}

		public:

			NbListen2<CLIENTSOCKET4> m_listen;
			EPReactor m_reactor;
	};
}

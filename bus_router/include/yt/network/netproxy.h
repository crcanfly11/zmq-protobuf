#pragma once

#include "yt/network/reactor.h"
#include "yt/network/listensocket.h"
#include "yt/network/datadecoder.h"
#include "yt/network/clientmap.h"
#include "yt/network/dataprocessor.h"
#include "yt/util/tsvector.h"
#include "yt/util/thread.h"

namespace yt
{
	//异步通讯的中心类
	template <class CLIENTSOCKET = ClientSocket>
		class NetProxy
		{
			public:
				NetProxy(const char *host,int port,int maxclientcount,int conntimeout,int maxinpacklen,int maxoutpacklen,DataDecoderBase *clientdatadecoder) : m_listener(host,port,&m_reactor,conntimeout,clientdatadecoder,&m_clientmap,maxclientcount,maxinpacklen,maxoutpacklen){}
				NetProxy(int fd,int maxclientcount,int conntimeout,int maxinpacklen,int maxoutpacklen,DataDecoderBase *clientdatadecoder) : m_listener(fd,&m_reactor,conntimeout,clientdatadecoder,&m_clientmap,maxclientcount,maxinpacklen,maxoutpacklen){}
				virtual ~NetProxy(){}
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
					if(m_listener.Listen() != 0)
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
					m_listener.Close();
					m_clientmap.Close();
				}
				inline Reactor* GetReactor(){
					return &m_reactor;
				}
				inline ClientMap* GetClientMap(){
					return &m_clientmap;
				}
			public:
			        EPReactor m_reactor;
				//EPETReactor m_reactor;
				ListenSocket<CLIENTSOCKET> m_listener;
				ClientMap m_clientmap;
		};
	//NetProxyQueueThreadsProcess的处理线程
	class ProcessThread : public Thread
	{
		public:
			ProcessThread(SyncQueue<pair<int,string>* > *processqueue,SyncQueue<pair<int,string>* > *outqueue,DataProcessor *processor,TSVector<long> *timelist);
			void Run();
		private:
			SyncQueue<pair<int,string>* > *m_processqueue;
			SyncQueue<pair<int,string>* > *m_outqueue;
			DataProcessor *m_processor;
			TSVector<long> *m_timelist;
	};
	//逻辑处理完后的异步通知类
	class SocketPairEventHandler : public FDEventHandler
	{
		public:
			SocketPairEventHandler(Reactor *pReactor,ClientMap *clientmap,SyncQueue<pair<int,string>* > *outqueue);
			~SocketPairEventHandler(){}
			virtual void OnFDRead();
			virtual void OnFDWrite(){};
			virtual void OnFDReadTimeOut(){};
			virtual void OnFDWriteTimeOut(){};
		private:
			ClientMap *m_clientmap;
			SyncQueue<pair<int,string>* > *m_outqueue;
	};
	//单线程处理网络，队列线程池处理的中心类
	class NetProxyQueueThreadsProcess : public NetProxy<>
	{
		public:
			NetProxyQueueThreadsProcess(const char *host,int port,int maxclientcount,int conntimeout,int maxoneinpacklen,int maxoutpacklen,DataProcessor *processor,int threadcount);
			virtual bool Init();
			virtual bool Start();
			virtual void Stop();
			virtual void UnInit();
			long GetSpeed();
			void SendAll(const char *buf,size_t buflen);
		public:
			TSVector<long> m_timelist;
			DataDecoder_Q clientdatadecoder;
			DataProcessor *m_processor;
			SyncQueue<pair<int,string>* > m_processqueue;
			SyncQueue<pair<int,string>* > m_outqueue;
			SocketPairEventHandler m_speh;
			int m_fd[2];
			SocketNotification m_sn;
			int m_threadcount;
			vector<Thread*> m_threadlist;
	};
	
	class NetworkThread;
	
	/** NotifyEventHandler **/
	class NotifyEventHandler : public FDEventHandler
	{
		public:
			NotifyEventHandler(Reactor *pReactor) : FDEventHandler(pReactor) {}
			
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

	class FdNotifyEventHandler : public NotifyEventHandler
	{
		public:
			FdNotifyEventHandler(Reactor *pReactor) : NotifyEventHandler(pReactor) {}
			
			void SetProcessHandler(NetworkThread *pMyThread) { m_pMyThread = pMyThread; }

			void SendNotify(const char* buf, size_t len);
			void SendNotify(int fd);
			void RecvNotify();

		private:
			NetworkThread *m_pMyThread;
	};

	class OnlyNotifyEventHandler : public NotifyEventHandler, public SocketNotification
	{
		public:
			OnlyNotifyEventHandler(Reactor *pReactor) : NotifyEventHandler(pReactor) {}

			bool Init();
			void SetProcessHandler(NetworkThread *pMyThread) { m_pMyThread = pMyThread; }

			void SendNotify(const char*, size_t) {}
			void RecvNotify();

			void notify();
			
		private:
			NetworkThread *m_pMyThread;
	};

	// 网络线程，共用输入队列，有自己单独的输出队列
	/** NetworkThread **/
	class NetworkThread : public Thread
	{
		public:
		    static const size_t CONNTIMEOUT = 360;                    // 连接超时
		    NetworkThread(int id, int maxclient, int maxoneinpacklen, int maxoutpacklen, SyncQueue<pair<int, pair<int,string>*> > *inqueue, int conntimeout = CONNTIMEOUT);

		    inline int StartThread() {return Thread::Start();}
		    virtual int CalcWeight();
		    virtual bool ApplyClient(int fd);
			void ProcessOutqueue();
			
		    virtual bool Init();	// 初始化
		    virtual bool Start();	// 启动
		    virtual void Stop();	// 停止
		    virtual void UnInit();	// 一些结束工作
		    virtual void Run();

		private:
		    void InsertClient(ClientSocket *client);

		public:
			int m_id;
			int m_maxclient;									// 线程处理的最大客户数
			int m_maxoneinpacklen;
			int m_maxoutpacklen;
			int m_conntimeout;
			
			DataDecoder_Q2 clientdatadecoder;					// 接收客户端请求包，并push到处理队列
			FdNotifyEventHandler m_listennotify;				// 与listen交互的notify
			OnlyNotifyEventHandler m_outqueuenotify;			// 与outqueue交互的notify
			SyncQueue<pair<int,string>* > m_outqueue;			// 单独的输出队列
			SyncQueue<pair<int, pair<int,string>*> > *m_inqueue;// 共用的输入队列

		    EPReactor m_reactor;
		    Counter m_counter;
		    ClientMap m_clientmap;
	};

	// 仅仅监听，并调用空闲网络线程的notify
	/** NbListen **/
	class NbListen : public ListenSocketBase
	{
		public:
		    NbListen(int fd, Reactor *pReactor, vector<NetworkThread*> *pthreadlist) : 
		        ListenSocketBase(fd, pReactor, 0), m_pthreadlist(pthreadlist), m_lastid(0) {}
			NbListen(const char *host,int port,Reactor *pReactor,vector<NetworkThread*> *pthreadlist) :
				ListenSocketBase(host, port, pReactor, 0), m_pthreadlist(pthreadlist), m_lastid(0) {}

		    virtual void OnAccept(int fd);

		private:
		    NetworkThread* SelectThread();

		private:
		    vector<NetworkThread*> *m_pthreadlist;
			int m_lastid;
	};

	/** ProcessThread2 **/
	class ProcessThread2 : public Thread
	{
		public:
			ProcessThread2(SyncQueue<pair<int, pair<int,string>*> > *processqueue, vector<NetworkThread*> *m_networkthreadlist, DataProcessor *processor, TSVector<long> *timelist);
			void Run();
		private:
			SyncQueue<pair<int, pair<int,string>*> > *m_processqueue;
			vector<NetworkThread*> *m_networkthreadlist;
			DataProcessor *m_processor;
			TSVector<long> *m_timelist;
	};

	// 多个网络线程，共用一个输入队列，有单独的输出队列
	/** NetProxyQueueThreadsProcess2 **/
	class NetProxyQueueThreadsProcess2
	{
		public:
			static const int NETWORK_THREAD_COUNT = 4;
			NetProxyQueueThreadsProcess2(const char *host,int port,int maxclientcount,int conntimeout,int maxoneinpacklen,int maxoutpacklen,DataProcessor *processor,int processthreadcount,int networkthreadcount = NETWORK_THREAD_COUNT);
			virtual ~NetProxyQueueThreadsProcess2() {}
			virtual bool Init();
			virtual bool Start();
			virtual void Stop();
			virtual void UnInit();
			long GetSpeed();
			void SendAll(const char *buf,size_t buflen);
			size_t GetClientSize();

		public:
			int m_maxclient;
			int m_conntimeout;
			int m_maxoneinpacklen;
			int m_maxoutpacklen;
			DataProcessor *m_processor;
			
			TSVector<long> m_timelist;
			int m_networkthreadcount;
			int m_processthreadcount;
			vector<Thread*> m_processthreadlist;
			vector<NetworkThread*> m_networkthreadlist;

			NbListen m_listen;
			EPReactor m_reactor;
			ClientMap m_clientmap;
			SyncQueue<pair<int, pair<int,string>*> > m_processqueue;
	};
}


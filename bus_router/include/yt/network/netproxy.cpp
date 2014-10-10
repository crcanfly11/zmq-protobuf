#include "yt/network/netproxy.h"
#include "yt/log/log.h"
#include "yt/util/timeuse.h"
#include <memory>
#include <limits.h>

namespace yt
{
/*
	template <class CLIENTSOCKET>
		NetProxy<CLIENTSOCKET>::NetProxy(const char *host,int port,int maxclientcount,int conntimeout,int maxinpacklen,int maxoutpacklen,DataDecoderBase *clientdatadecoder) : m_listener(host,port,&m_reactor,conntimeout,clientdatadecoder,&m_clientmap,maxclientcount,maxinpacklen,maxoutpacklen){}

	template <class CLIENTSOCKET>
		NetProxy<CLIENTSOCKET>::NetProxy(int fd,int maxclientcount,int conntimeout,int maxinpacklen,int maxoutpacklen,DataDecoderBase *clientdatadecoder) : m_listener(fd,&m_reactor,conntimeout,clientdatadecoder,&m_clientmap,maxclientcount,maxinpacklen,maxoutpacklen){}

	template <class CLIENTSOCKET>
		NetProxy<CLIENTSOCKET>::~NetProxy(){}

	template <class CLIENTSOCKET>
		bool NetProxy<CLIENTSOCKET>::Init()
		{
			if(m_reactor.Init() != 0)
			{
				AC_ERROR("reactor init error");
				return false;
			}
			
			return true;
		}
	template <class CLIENTSOCKET>
		bool NetProxy<CLIENTSOCKET>::Start()
		{
			if(m_listener.Listen() != 0)
			{
				AC_ERROR("main listen error");
				return false;
			}
			m_reactor.Run();
			return true;
		}
	template <class CLIENTSOCKET>
		void NetProxy<CLIENTSOCKET>::Stop()
		{
			m_reactor.Stop();
		}
	template <class CLIENTSOCKET>
		void NetProxy<CLIENTSOCKET>::UnInit()
		{
			m_listener.Close();	//关闭侦听套接口
			m_clientmap.Close();	//关闭所有客户端
		}
*/
	ProcessThread::ProcessThread(SyncQueue<pair<int,string>* > *processqueue,SyncQueue<pair<int,string>* > *outqueue,DataProcessor *processor,TSVector<long> *timelist) : m_processqueue(processqueue),m_outqueue(outqueue),m_processor(processor),m_timelist(timelist){}
	void ProcessThread::Run()
	{
		while(1)
		{
			pair<int,string> *inpair;
			m_processqueue->Pop(inpair,true);
			auto_ptr<pair<int,string> > g(inpair);
			if(inpair->first < 0)
			{
				AC_INFO("stop process thread");
				break;
			}

			timeval begin;gettimeofday(&begin,NULL);
			
			pair<int,string> *outpair = new pair<int,string>;
			int ret = m_processor->Process(inpair->second.data(),inpair->second.length(),outpair->second);

			if(ret == 0)//处理正确，产生输出包
			{
				outpair->first = inpair->first;
			}
			else if(ret == 1)//处理正确，不需要产生输出包
			{
				delete outpair;
				continue;
			}
			else	//出错
			{
				delete outpair;
				AC_ERROR("process error.[ret:%d]", ret);
				continue;
			}

			if(m_outqueue->Push(outpair) < 0)
			{
				delete outpair;
				AC_ERROR("push result queue error");
				//Alarm3(AT_RESULTQUEUEFULL,"push result queue error");
			}

			timeval end;gettimeofday(&end,NULL);
			
			//统计速度
			m_timelist->Acquire();
			if(m_timelist->size() > 1000)
			{
				m_timelist->erase(m_timelist->begin());
			}
			m_timelist->push_back(time_use(&begin,&end));
			m_timelist->Release();
			//--------------
		}
	}
	SocketPairEventHandler::SocketPairEventHandler(Reactor *pReactor,ClientMap *clientmap,SyncQueue<pair<int,string>* > *outqueue) : FDEventHandler(pReactor),m_clientmap(clientmap),m_outqueue(outqueue){}
	void SocketPairEventHandler::OnFDRead()
	{
		char buf[64];
		recv(m_fd,buf,sizeof(buf),0);
		//AC_INFO("%s",buf);
		while(1)
		{
			pair<int,string> *p;
			if(m_outqueue->Pop(p,false) != 0)
				break;
			auto_ptr<pair<int,string> > g(p);
			
			if(p->first < 0)
			{
				/*int clientid = atoi(p->second.c_str());
				ClientSocket *clientsocket = m_clientmap->Get(clientid);
				if(clientsocket)
					clientsocket->Close();*/
				m_clientmap->SendAll(p->second.data(),p->second.length());
			}
			else
			{
				int clientid = p->first;
				ClientSocket *clientsocket = m_clientmap->Get(clientid);
				if(clientsocket)
					clientsocket->SendBuf(p->second.data(),p->second.length());
			}
		}
	}
	NetProxyQueueThreadsProcess::NetProxyQueueThreadsProcess(const char *host,int port,int maxclientcount,int conntimeout,int maxoneinpacklen,int maxoutpacklen,DataProcessor *processor,int threadcount) : NetProxy<>(host,port,maxclientcount,conntimeout,10 * maxoneinpacklen,maxoutpacklen,&clientdatadecoder),clientdatadecoder(maxoneinpacklen),m_processor(processor),m_processqueue(20000),m_speh(&m_reactor,&m_clientmap,&m_outqueue),m_threadcount(threadcount)
	{
		clientdatadecoder.SetQueue(&m_processqueue);
	}
	bool NetProxyQueueThreadsProcess::Init()
	{
		if(!NetProxy<>::Init())
			return false;

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
		m_speh.SetFD(m_fd[1]);
		if(m_speh.RegisterRead((struct timeval*)NULL) != 0)
		{
			AC_ERROR("register socket pair error");
			return false;
		}
		m_sn.SetFD(m_fd[0]);
		m_outqueue.SetNotification(&m_sn);
		return true;
	}
	bool NetProxyQueueThreadsProcess::Start()
	{
		if(m_listener.Listen() != 0)
		{
			AC_ERROR("main listen error");
			return false;
		}
		for(int i = 0;i < m_threadcount;i++)
		{
			ProcessThread *thread = new ProcessThread(&m_processqueue,&m_outqueue,m_processor,&m_timelist);
			if(thread->Start() == -1)
				return false;
			m_threadlist.push_back(thread);
		}
		m_reactor.Run();
		return true;
		//return NetProxy<>::Start();
	}
	void NetProxyQueueThreadsProcess::Stop()
	{
		NetProxy<>::Stop();
	}
	void NetProxyQueueThreadsProcess::UnInit()
	{
		close(m_fd[0]);
		close(m_fd[1]);
		NetProxy<>::UnInit();

		for(size_t i = 0;i < m_threadlist.size();i++)
		{
			pair<int,string> *p = new pair<int,string>(-1,"");
			m_processqueue.Push(p);
		}
		
		for(size_t i = 0;i < m_threadlist.size();i++)
		{
			m_threadlist[i]->Join();
			delete m_threadlist[i];
		}
	}
	long NetProxyQueueThreadsProcess::GetSpeed()
	{
		m_timelist.Acquire();
		LockGuard g(&m_timelist);

		if(m_timelist.size() == 0)
			return 0;
	
		long sum = 0;
		for(size_t i = 0;i < m_timelist.size();i++)
		{
			sum += m_timelist[i];
		}
		return (long)((double)sum / (double)m_timelist.size());
	}
	void NetProxyQueueThreadsProcess::SendAll(const char *buf,size_t buflen)
	{
		pair<int,string> *outpair = new pair<int,string>;
		outpair->first = -1;
		outpair->second.append(buf,buflen);
		if(m_outqueue.Push(outpair) < 0)
		{
			delete outpair;
			AC_ERROR("push result queue error");
			//Alarm3(AT_RESULTQUEUEFULL,"push result queue error");
		}
	}
	
	/** NetProxyQueueThreadsProcess2 **/
	NetProxyQueueThreadsProcess2::NetProxyQueueThreadsProcess2(const char *host,int port,int maxclientcount,int conntimeout,int maxoneinpacklen,int maxoutpacklen,DataProcessor *processor,int processthreadcount,int networkthreadcount) : m_maxclient(maxclientcount), m_conntimeout(conntimeout), m_maxoneinpacklen(maxoneinpacklen), m_maxoutpacklen(maxoutpacklen), m_processor(processor), m_networkthreadcount(networkthreadcount), m_processthreadcount(processthreadcount),
			m_listen(host, port, &m_reactor, &m_networkthreadlist), m_processqueue(20000) {}

	bool NetProxyQueueThreadsProcess2::Init()
	{
		// init reactor
		if(m_reactor.Init() != 0)
		{
			AC_ERROR("reactor init error");
			return false;
		}

		// init network thread
		int maxclient = (m_maxclient + m_networkthreadcount - 1) / m_networkthreadcount;		// 实际接入线程数会略大于m_maxclient
		for(int i = 0;i < m_networkthreadcount;i++)
		{
			NetworkThread *thread = new NetworkThread(i, maxclient, m_maxoneinpacklen, m_maxoutpacklen, &m_processqueue, m_conntimeout);
			
			if(thread->Init() == -1)
				return false;
			m_networkthreadlist.push_back(thread);
		}

		// init process thread
		for(int i = 0;i < m_processthreadcount;i++)
		{
			ProcessThread2 *thread = new ProcessThread2(&m_processqueue,&m_networkthreadlist,m_processor,&m_timelist);
			m_processthreadlist.push_back(thread);
		}

		return true;
	}
	bool NetProxyQueueThreadsProcess2::Start()
	{
		if(m_listen.Listen() != 0)
		{
			AC_ERROR("main listen error");
			return false;
		}

		for(size_t i = 0;i < m_networkthreadlist.size();i++)
		{
			m_networkthreadlist[i]->StartThread();
		}
		
		for(size_t i = 0;i < m_processthreadlist.size();i++)
		{
			m_processthreadlist[i]->Start();
		}
		
		m_reactor.Run();
		
		return true;
	}
	
	void NetProxyQueueThreadsProcess2::Stop()
	{
		m_listen.Close();
		m_reactor.Stop();
	}
	
	void NetProxyQueueThreadsProcess2::UnInit()
	{
		// close process thread
		for(size_t i = 0;i < m_processthreadlist.size();i++)
		{
			m_processqueue.Push(make_pair(-1, (pair<int,string>*)NULL));
		}

		for(size_t i = 0;i < m_processthreadlist.size();i++)
		{
			m_processthreadlist[i]->Join();
			delete m_processthreadlist[i];
		}
		m_processthreadlist.clear();
		
		// close network thread
		for(size_t i = 0;i < m_networkthreadlist.size();i++)
		{
			m_networkthreadlist[i]->Stop();
		}

		for(size_t i = 0;i < m_networkthreadlist.size();i++)
		{
			m_networkthreadlist[i]->Join();
			delete m_networkthreadlist[i];
		}
		m_networkthreadlist.clear();
	}
	
	long NetProxyQueueThreadsProcess2::GetSpeed()
	{
		m_timelist.Acquire();
		LockGuard g(&m_timelist);

		if(m_timelist.size() == 0)
			return 0;

		long sum = 0;
		for(size_t i = 0;i < m_timelist.size();i++)
		{
			sum += m_timelist[i];
		}
		return (long)((double)sum / (double)m_timelist.size());
	}
	
	size_t NetProxyQueueThreadsProcess2::GetClientSize()
	{
		size_t count = 0;

		for(size_t i = 0;i < m_networkthreadlist.size();i++)
		{
			count += m_networkthreadlist[i]->m_clientmap.Size();
		}
		
		return count;
	}
	
	void NetProxyQueueThreadsProcess2::SendAll(const char *buf,size_t buflen)
	{
		for(size_t i = 0;i < m_networkthreadlist.size();i++)
		{
			pair<int,string> *outpair = new pair<int,string>;
			outpair->first = -1;
			outpair->second.append(buf,buflen);
			if(m_networkthreadlist[i]->m_outqueue.Push(outpair) < 0)
			{
				delete outpair;
				AC_ERROR("push result queue error");
				//Alarm3(AT_RESULTQUEUEFULL,"push result queue error");
			}
		}
	}
	
	/** NbListen **/
	void NbListen::OnAccept(int fd)
	{
	    // 选择一个当前最恰当的proxy
	    NetworkThread *thread = SelectThread();
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

	NetworkThread* NbListen::SelectThread()
	{
	    int minweight = INT_MAX;
	    NetworkThread* thread = NULL;
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

	/** NetworkThread **/
	NetworkThread::NetworkThread(int id, int maxclient, int maxoneinpacklen, int maxoutpacklen, SyncQueue<pair<int, pair<int,string>*> > *inqueue, int conntimeout) :
			m_id(id), m_maxclient(maxclient), m_maxoneinpacklen(maxoneinpacklen), m_maxoutpacklen(maxoutpacklen), m_conntimeout(conntimeout), clientdatadecoder(maxoneinpacklen), m_listennotify(&m_reactor), m_outqueuenotify(&m_reactor), m_inqueue(inqueue)
	{
		m_listennotify.SetProcessHandler(this);
		m_outqueuenotify.SetProcessHandler(this);

		clientdatadecoder.SetQueue(m_inqueue);
		clientdatadecoder.SetId(m_id);
	}

	bool NetworkThread::ApplyClient(int fd)
	{
	    // 检查上限
	    if((int)m_clientmap.Size() > m_maxclient)
	    {
	        AC_ERROR("too many client");
	        close(fd);
	        return false;
	    }

	    // proxy的逻辑处理
	    ClientSocket *pClient = new(std::nothrow) ClientSocket(&m_reactor, &clientdatadecoder, &m_clientmap, fd, 10 * m_maxoneinpacklen, m_maxoutpacklen); //创建客户端对象
	    if (!pClient)
	    {
			AC_ERROR("Cann't Allocate client in pool");
			Alarm3(AT_MEMERROR,"cann't allocate client");
	        close(fd);
	        return false;
	    }

	    // 插入clientmap
	    InsertClient(pClient);
		
		AC_INFO("network thread[%d], count: [%d]\n", m_id, m_clientmap.Size());

	    return true;
	}
	
	void NetworkThread::InsertClient(ClientSocket *pClient)
	{
	    int seq = m_counter.Get();
	    while(m_clientmap.Get(seq) != NULL)
	    {
	        AC_INFO("seq overlap");
	        seq = m_counter.Get();
	    }

	    pClient->SetClientID(seq);	            // 设置客户端的id
	    m_clientmap.Put(seq, pClient);	        // 放入clientmap

	   	if(pClient->RegisterRead(m_conntimeout) != 0)	//注册读事件
	    {
	        AC_ERROR("register read error");
	        pClient->Close();
	        return;
	    }
	}

	void NetworkThread::Run()
	{
	    AC_INFO("network thread start.");
		
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

	bool NetworkThread::Init()
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

		if (!m_outqueuenotify.Init())
	    {
	        AC_ERROR("init notify error.");
	        return false;
	    }

		m_outqueue.SetNotification(&m_outqueuenotify);
		
	    return true;
	}

	bool NetworkThread::Start()
	{
	    m_reactor.Run();
	    return true;
	}

	void NetworkThread::Stop()
	{
	    m_reactor.Stop();
	}

	void NetworkThread::UnInit()
	{
	    m_clientmap.Close();			//关闭所有客户端
	    m_listennotify.Close();
	    m_outqueuenotify.Close();
	}

	int NetworkThread::CalcWeight()
	{
		if (m_clientmap.Size() >= (size_t)m_maxclient)
			return INT_MAX;

		return m_clientmap.Size();
	}

	void NetworkThread::ProcessOutqueue()
	{
		while(1)
		{
			pair<int,string> *p;
			if(m_outqueue.Pop(p,false) != 0)
				break;
			auto_ptr<pair<int,string> > g(p);
			
			if(p->first < 0)
			{
				/*int clientid = atoi(p->second.c_str());
				ClientSocket *clientsocket = m_clientmap->Get(clientid);
				if(clientsocket)
					clientsocket->Close();*/
				m_clientmap.SendAll(p->second.data(),p->second.length());
			}
			else
			{
				int clientid = p->first;
				ClientSocket *clientsocket = m_clientmap.Get(clientid);

				if(clientsocket)
					clientsocket->SendBuf(p->second.data(),p->second.length());
			}
		}
	}

	ProcessThread2::ProcessThread2(SyncQueue<pair<int, pair<int,string>*> > *processqueue, vector<NetworkThread*> *networkthreadlist, DataProcessor *processor, TSVector<long> *timelist) : 
		m_processqueue(processqueue), m_networkthreadlist(networkthreadlist), m_processor(processor), m_timelist(timelist) {}
		
	void ProcessThread2::Run()
	{
		AC_INFO("process thread start.");
	
		while(1)
		{
			pair<int, pair<int,string>*> t;
			m_processqueue->Pop(t,true);
			int threadid = t.first;

			pair<int,string> *inpair = t.second;
			auto_ptr<pair<int,string> > g(inpair);
			if(threadid < 0)
			{
				AC_INFO("stop process thread");
				break;
			}

			timeval begin;gettimeofday(&begin,NULL);
			
			pair<int,string> *outpair = new pair<int,string>;
			int ret = m_processor->Process(inpair->second.data(),inpair->second.length(),outpair->second);

			if(ret == 0)//处理正确，产生输出包
			{
				outpair->first = inpair->first;
			}
			else if(ret == 1)//处理正确，不需要产生输出包
			{
				delete outpair;
				continue;
			}
			else	//出错
			{
				delete outpair;
				AC_ERROR("process error.[ret:%d]", ret);
				continue;
			}

			if((*m_networkthreadlist)[threadid]->m_outqueue.Push(outpair) < 0)
			{
				delete outpair;
				AC_ERROR("push result queue error");
				//Alarm3(AT_RESULTQUEUEFULL,"push result queue error");
			}

			timeval end;gettimeofday(&end,NULL);
			
			//统计速度
			m_timelist->Acquire();
			if(m_timelist->size() > 1000)
			{
				m_timelist->erase(m_timelist->begin());
			}
			m_timelist->push_back(time_use(&begin,&end));
			m_timelist->Release();
			//--------------
		}
	}
	
	bool NotifyEventHandler::Init()
	{
		// socket
		if(socketpair(AF_UNIX, SOCK_STREAM, 0, m_fdpair) < 0)
		{
			AC_ERROR("socketpair error");
			return false;
		}

		// set non block(仅读)
		if(FDEventHandler::SetNonBlocking(m_fdpair[1],true) != 0)
		{
			AC_ERROR("set non blocing error");
			return false;
		}

		// set fd
		FDEventHandler::SetFD(m_fdpair[1]);			// 读

		// register
		if(FDEventHandler::RegisterRead((struct timeval*)NULL) != 0)
		{
			AC_ERROR("register socket pair error");
			return false;
		}
		
		return true;
	}

	void NotifyEventHandler::Close() 
	{
		close(m_fdpair[0]);
		FDEventHandler::Close();
	}

	void FdNotifyEventHandler::SendNotify(const char* buf, size_t len)
	{
		// 阻塞发送
		if (send(GetSendFD(), buf, len, 0) != (ssize_t)len)
			AC_ERROR("send fd error.");
	}

	void FdNotifyEventHandler::SendNotify(int fd)
	{
		SendNotify((char*)(&fd), sizeof(int));
	}
	
	void FdNotifyEventHandler::RecvNotify()
	{
		int fd;
		
		if (recv(FDEventHandler::m_fd, (char*)&fd, sizeof(int), 0) != sizeof(int))
			AC_ERROR("recv fd error.");

		if (!m_pMyThread->ApplyClient(fd))
		{
			AC_ERROR("apply client error.");
		}
	}

	bool OnlyNotifyEventHandler::Init()
	{
		bool ret = NotifyEventHandler::Init();
		if (!ret)
			return false;
		
		SocketNotification::SetFD(GetSendFD()); 	// 写
		return true;
	}

	void OnlyNotifyEventHandler::notify()
	{
		send(SocketNotification::GetFD(), "A", 1, MSG_DONTWAIT);
	}
	
	void OnlyNotifyEventHandler::RecvNotify()
	{
		char buf[64];
		recv(FDEventHandler::m_fd,buf,sizeof(buf),0);
	
		m_pMyThread->ProcessOutqueue();
	}
}


#include "yt/network/netproxythreadcluster.h"
#include "yt/log/log.h"
#include "yt/util/timeuse.h"
#include <memory>

namespace yt
{
	WorkThread::WorkThread(SyncQueue<pair<int,string>* > *outqueue,DataProcessor *processor,TSVector<long> *timelist,ThreadCluster *threadcluster,bool bpermanet):m_outqueue(outqueue),m_processor(processor),m_timelist(timelist),m_taskstat(0),m_threadcluster(threadcluster),m_bpermanet(bpermanet)
	{
		m_starttm = time(NULL);
		pthread_mutex_init(&m_mutex,NULL);
		pthread_cond_init(&m_cond,NULL);
	}
	WorkThread::~WorkThread()
	{
		pthread_mutex_destroy(&m_mutex);
		pthread_cond_destroy(&m_cond);
	}
	void WorkThread::Run()
	{
		while(1)
		{
			WaitForTask();
			if(m_clientid < 0)
			{
				//AC_INFO("stop process thread");
				break;
			}

			pair<int,string> *outdata = new pair<int,string>;
			timeval begin;gettimeofday(&begin,NULL);
			
			int ret = m_processor->Process(m_request.data(),m_request.length(),outdata->second);

			if(ret == 0)//处理正确，产生输出包
			{
				outdata->first = m_clientid;
				if(outdata&&(m_outqueue->Push(outdata) < 0))
				{
					delete outdata;
					AC_ERROR("push result queue error");
				}
			}
			else if(ret == 1)//处理正确，不需要产生输出包
			{
				delete outdata;
			}
			else	//出错
			{
				delete outdata;
				AC_ERROR("process error.[ret:%d]", ret);
			}
			m_request.clear();
			timeval end;gettimeofday(&end,NULL);
			//统计速度
			m_timelist->Acquire();
			if(m_timelist->size() > 1000)
			{
				m_timelist->erase(m_timelist->begin());
			}
			m_timelist->push_back(time_use(&begin,&end));
			m_timelist->Release();
			m_threadcluster->OnComplete(this);
		}
	}
	void WorkThread::Schedule(int clientid, const char *buf,size_t buflen)
	{
		MutexGard gard(&m_mutex);
		m_taskstat --;
		if(m_taskstat<0)
		{
			pthread_cond_wait(&m_cond,&m_mutex);
		}
		m_clientid = clientid;
		m_request.clear();
		m_request.assign(buf,buflen);
		pthread_cond_signal(&m_cond);
	}
	void WorkThread::WaitForTask()
	{
		MutexGard gard(&m_mutex);
		if(m_taskstat<0)
			pthread_cond_signal(&m_cond);
		m_taskstat ++;
		pthread_cond_wait(&m_cond,&m_mutex);
	}

	ThreadCluster::ThreadCluster(SyncQueue<pair<int,string>* > *outqueue,TSVector<long> *timelist,DataProcessor *processor,int threadcount,int maxthreadcount,int threadto):m_outqueue(outqueue),m_timelist(timelist),m_processor(processor),m_threadcount(threadcount),m_maxthreadcount(maxthreadcount),m_threadto(threadto)
	{
	}
	 ThreadCluster::~ThreadCluster()
	 {
	 }

	bool ThreadCluster::Init()
	{
		if(m_maxthreadcount < m_threadcount)
		{
			AC_ERROR("err threadcount > maxthreadcount");
			return false;
		}
		m_freethread.Acquire();
		LockGuard gard(&m_freethread);
		for(int i = 0 ; i < m_threadcount ; i++)
		{
			WorkThread *thread = new WorkThread(m_outqueue,m_processor,m_timelist,this);
			thread->Start();
			m_freethread.push_back(thread);
		}
		return true;
	}
	void ThreadCluster::UnInit()
	{
		while(1)
		{
			bool emptya,emptyb;
			m_workingthread.Acquire();
			emptya = m_workingthread.empty();
			m_workingthread.Release();
			
			if(emptya)
			{
				m_temporaryworkingthread.Acquire();
				emptyb = m_temporaryworkingthread.empty();
				m_temporaryworkingthread.Release();
				if(emptyb)
					break;
			}
			usleep(200);
		}
	
		list<WorkThread*>::iterator iter;
		m_freethread.Acquire();
		for(iter = m_freethread.begin(); iter != m_freethread.end() ; iter++)
		{
			WorkThread *thread =(WorkThread*)(*iter);
			thread->Schedule(-1,NULL,0);
			thread->Join();
			delete thread;
		}
		m_freethread.Release();
		m_temporaryfreethread.Acquire();
		for(iter = m_temporaryfreethread.begin(); iter != m_temporaryfreethread.end() ; iter++)
		{
			WorkThread *thread =(WorkThread*)(*iter);
			thread->Schedule(-1,NULL,0);
			thread->Join();
			delete thread;
		}
		m_temporaryfreethread.Release();
		
	}
	int ThreadCluster::Handle(int clientid, const char *buf,size_t buflen)
	{
		WorkThread *thread;
		if(-1 == GetFreeThread(thread))
		{
			return -1;
		}
		thread->Schedule(clientid,buf,buflen);
		return 0;
	}
	void ThreadCluster::OnComplete(WorkThread *thread)
	{
		if(thread->IsPermanet())
		{
			m_workingthread.Acquire();
			m_workingthread.remove(thread);
			m_workingthread.Release();
			m_freethread.Acquire();
			m_freethread.push_back(thread);
			m_freethread.Release();
		}
		else
		{
			m_temporaryworkingthread.Acquire();
			m_temporaryworkingthread.remove(thread);
			m_temporaryworkingthread.Release();
			m_temporaryfreethread.Acquire();
			m_temporaryfreethread.push_back(thread);
			m_temporaryfreethread.Release();
		}
	}
	void ThreadCluster::OnTaskCheck()
	{
		m_temporaryfreethread.Acquire();
		LockGuard gard(&m_temporaryfreethread);
		list<WorkThread*>::iterator iter;
		time_t currenttm = time(NULL);
		for(iter = m_temporaryfreethread.begin(); iter != m_temporaryfreethread.end();)
		{
			int timeuse = currenttm - (*iter)->m_starttm;
			if(timeuse >= m_threadto)
			{
				(*iter)->Schedule(-1,NULL,0);
				(*iter)->Join();
				delete *iter;
				iter = m_temporaryfreethread.erase(iter);
			}
			else
			{
				iter ++;
			}
		}
		
	}

	int ThreadCluster::GetFreeThread(WorkThread *&thread)
	{
		
		m_freethread.Acquire();
		if(!m_freethread.empty())
		{
			thread = *m_freethread.begin();
			m_freethread.pop_front();
			m_freethread.Release();
			m_workingthread.Acquire();
			m_workingthread.push_back(thread);
			m_workingthread.Release();
			return 0;
		}
		m_freethread.Release();
		m_temporaryfreethread.Acquire();
		if(!m_temporaryfreethread.empty())
		{
			thread = *m_temporaryfreethread.begin();
			m_temporaryfreethread.pop_front();
			m_temporaryfreethread.Release();
			m_temporaryworkingthread.Acquire();
			m_temporaryworkingthread.push_back(thread);
			m_temporaryworkingthread.Release();
			return 0;
		}
		m_temporaryfreethread.Release();
		m_temporaryworkingthread.Acquire();
		m_temporaryfreethread.Acquire();
		int count = m_threadcount + m_temporaryworkingthread.size() + m_temporaryfreethread.size();
		m_temporaryfreethread.Release();
		m_temporaryworkingthread.Release();
		if(count >= m_maxthreadcount)
		{
			AC_ERROR("no enough working thread");
			return -1;
		}
		thread = new WorkThread(m_outqueue,m_processor,m_timelist,this,false);
		thread->Start();
		m_temporaryworkingthread.Acquire();
		m_temporaryworkingthread.push_back(thread);
		m_temporaryworkingthread.Release();
		return 0;
	}
	ThreadClusterSocketPairHandler::ThreadClusterSocketPairHandler(Reactor *pReactor,ClientMap *clientmap,SyncQueue<pair<int,string>* > *outputqueue,ThreadCluster *threadcluster) : FDEventHandler(pReactor),m_clientmap(clientmap),m_outputqueue(outputqueue),m_threadcluster(threadcluster){}
	void ThreadClusterSocketPairHandler::OnFDRead()
	{
		char buf[64];
		recv(m_fd,buf,sizeof(buf),0);
		//AC_INFO("%s",buf);
		while(1)
		{
			pair<int,string>  *p;
			if(m_outputqueue->Pop(p,false) != 0)
				break;
			auto_ptr<pair<int,string> > g(p);
			
			if(p->first == -1)
			{
				m_clientmap->SendAll(p->second.data(),p->second.length());
			}
			else
			{
				if(p->first >= 0)
				{
					int clientid = p->first;
					ClientSocket *clientsocket = m_clientmap->Get(clientid);
					if(clientsocket)
						clientsocket->SendBuf(p->second.data(),p->second.length());
				}
			}
		}
	}

	void ThreadClusterTimer::OnTimeOut()
	{
		m_threadcluster->OnTaskCheck();
	}
	
	NetProxyThreadClusterProcess::NetProxyThreadClusterProcess(const char *host,int port,int maxclientcount,int conntimeout,int maxoneinpacklen,int maxoutpacklen,DataProcessor *processor,int threadcount,int maxthreadcount,int taskcheckto) : NetProxy<>(host,port,maxclientcount,conntimeout,10 * maxoneinpacklen,maxoutpacklen,&m_clientdatadecoder),m_clientdatadecoder(maxoneinpacklen),m_outqueue(maxthreadcount),m_threadcluster(&m_outqueue,&m_timelist,processor,threadcount,maxthreadcount,taskcheckto),m_speh(&m_reactor,&m_clientmap,&m_outqueue,&m_threadcluster),m_timer(&m_reactor,&m_threadcluster),m_taskcheckto(taskcheckto)
	{
		m_clientdatadecoder.SetThreadCluster(&m_threadcluster);
	}
	bool NetProxyThreadClusterProcess::Init()
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
		if(!m_threadcluster.Init())
		{
			AC_ERROR("fail to init threadcluster!");
			return false;
		}
		m_timer.RegisterTimer(m_taskcheckto);
		return true;
	}
	bool NetProxyThreadClusterProcess::Start()
	{
		if(m_listener.Listen() != 0)
		{
			AC_ERROR("main listen error");
			return false;
		}
		m_reactor.Run();
		return true;
		//return NetProxy<>::Start();
	}
	void NetProxyThreadClusterProcess::Stop()
	{
		NetProxy<>::Stop();
	}
	void NetProxyThreadClusterProcess::UnInit()
	{
		close(m_fd[0]);
		close(m_fd[1]);
		NetProxy<>::UnInit();

		m_threadcluster.UnInit();
	}
	long NetProxyThreadClusterProcess::GetSpeed()
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
	void NetProxyThreadClusterProcess::SendAll(const char *buf,size_t buflen)
	{
		pair<int,string> *out = new pair<int,string>;
		out->first= -1;
		out->second.append(buf,buflen);
		if(m_outqueue.Push(out) < 0)
		{
			delete out;
			AC_ERROR("push result queue error");
			//Alarm3(AT_RESULTQUEUEFULL,"push result queue error");
		}
	}
	
}


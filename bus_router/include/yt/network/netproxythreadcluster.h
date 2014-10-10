#pragma once

#include "yt/network/netproxy.h"

namespace yt
{
	class ThreadCluster;
	class WorkThread : public Thread
	{
		public:
			WorkThread(SyncQueue<pair<int,string>* > *outqueue,DataProcessor *processor,TSVector<long> *timelist,ThreadCluster *threadcluster,bool bpermanet = true);
			~WorkThread();
			void Run();
			void Schedule(int clientid, const char *buf,size_t buflen);
			
			bool IsPermanet(){return m_bpermanet;};
			time_t m_starttm;
		private:
			void WaitForTask();
			class MutexGard
			{
				public:
					MutexGard(pthread_mutex_t *mutex):m_mutex(mutex)
					{
						pthread_mutex_lock(m_mutex);
					}
					~MutexGard()
					{
						pthread_mutex_unlock(m_mutex);
					}
					pthread_mutex_t *m_mutex;
			};
			pthread_mutex_t m_mutex;
			pthread_cond_t m_cond;
			SyncQueue<pair<int,string>* > *m_outqueue;
			DataProcessor *m_processor;
			TSVector<long> *m_timelist;
			int m_clientid;
			string m_request;
			int m_taskstat;
			ThreadCluster *m_threadcluster;
			bool m_bpermanet;
	};
	class ThreadCluster
	{
		public:
			ThreadCluster(SyncQueue<pair<int,string>* > *outqueue,TSVector<long> *timelist,DataProcessor *processor,int threadcount,int maxthreadcount,int threadto);
			virtual ~ThreadCluster();

			bool Init();
			void UnInit();
			int Handle(int clientid, const char *buf,size_t buflen);
			void OnComplete(WorkThread *thread);
			void OnTaskCheck();

			int GetFreeThread(WorkThread *&thread);
			TSList<WorkThread*> m_freethread;
			TSList<WorkThread*> m_workingthread;
			TSList<WorkThread*> m_temporaryfreethread;
			TSList<WorkThread*> m_temporaryworkingthread;
			SyncQueue<pair<int,string>* > *m_outqueue;
			TSVector<long> *m_timelist;
			DataProcessor *m_processor;
			int m_threadcount;
			int m_maxthreadcount;
			int m_threadto;
	};
	class ThreadClusterTimer : public TMEventHandler
	{
		public:
			ThreadClusterTimer(Reactor* preactor,ThreadCluster *threadcluster) : TMEventHandler(preactor),m_threadcluster(threadcluster){};
			
			void OnTimeOut();
		private:
			ThreadCluster *m_threadcluster;
	};

	class ThreadClusterSocketPairHandler : public FDEventHandler
	{
		public:
			ThreadClusterSocketPairHandler(Reactor *pReactor,ClientMap *clientmap,SyncQueue<pair<int,string> *> *outputqueue,ThreadCluster *threadcluster);
			~ThreadClusterSocketPairHandler(){}
			virtual void OnFDRead();
			virtual void OnFDWrite(){};
			virtual void OnFDReadTimeOut(){};
			virtual void OnFDWriteTimeOut(){};
		private:
			ClientMap *m_clientmap;
			SyncQueue<pair<int,string> *> *m_outputqueue;
			ThreadCluster *m_threadcluster;
	};
	class NetProxyThreadClusterProcess : public NetProxy<>
	{
		public:
			NetProxyThreadClusterProcess(const char *host,int port,int maxclientcount,int conntimeout,int maxoneinpacklen,int maxoutpacklen,DataProcessor *processor,int threadcount,int maxthreadcount = 1024,int taskcheckto = 60);
			virtual bool Init();
			virtual bool Start();
			virtual void Stop();
			virtual void UnInit();
			long GetSpeed();
			void SendAll(const char *buf,size_t buflen);
		public:
			TSVector<long> m_timelist;
			DataDecoder_Cluster m_clientdatadecoder;
			SyncQueue<pair<int,string> *> m_outqueue;
			int m_fd[2];
			SocketNotification m_sn;
			ThreadCluster m_threadcluster;
			ThreadClusterSocketPairHandler m_speh;
			ThreadClusterTimer m_timer;
			int m_taskcheckto;
	};
	
}


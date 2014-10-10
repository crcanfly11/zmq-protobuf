#pragma once

#include <yt/util/non_copyable.h>
#include <yt/util/thread.h>
#include <pthread.h>
#include <list>

using namespace std;

namespace yt 
{
	//线程池类，其实没用到
	class JobThread;

	// NOTICE: ThreadPool
	/*class ThreadPool  : public NonCopyable
	{
		protected:
			typedef std::list<Thread*>	ThreadPtrContainer;
			ThreadPtrContainer	livethreads;
			ThreadPtrContainer	deadthreads;
			pthread_mutex_t		mutex;
			volatile size_t			stopcount;
		public:
			ThreadPool();
			virtual ~ThreadPool();
			int Start(size_t count);
			void Stop(size_t count);
			void StopAll();
			size_t GetThreadNum();
		protected:
			virtual bool RunOnce();
			virtual void StopThreads(size_t stopcount);
		protected:
			Thread* CreateThread();
			void DestroyThread(Thread* thread);
			friend class JobThread;
	};*/

	class ThreadPool : public NonCopyable
	{
		friend class JobThread;
		public:
			virtual ~ThreadPool(){}
			int Start(size_t threadcount);
			void Join();
			Thread* CreateThread();
		private:
			virtual bool RunOnce();
			list<Thread*> m_threadlist;
	};

} // namespace yt


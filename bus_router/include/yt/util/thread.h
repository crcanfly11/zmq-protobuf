#pragma once

#include <pthread.h>
#include <yt/util/non_copyable.h>

namespace yt 
{
	//线程类，用到线程都可以继承这个类
	class Thread : public NonCopyable
	{
		public:
			Thread();
			virtual ~Thread(){}
			int Start(int detachstate = PTHREAD_CREATE_JOINABLE);
			void Cancel();
			void Join();
			inline pthread_t GetThreadID() const { return hdl; }
		private:
			virtual void Run() = 0;
		private:
			pthread_t hdl;

			friend void* __THREAD_FUNC(void* p);
	};
}

#pragma once

#include <pthread.h>
#include "lockable.h"

namespace yt
{
	//∂¡–¥À¯¿‡
	class RWMutex : public Lockable
	{
		public:
			virtual int AcquireRead(bool block = true) = 0;
			virtual int AcquireWrite(bool block = true) = 0;
	};
	class ThreadRWMutex : public RWMutex
	{
		public:
			ThreadRWMutex();
			~ThreadRWMutex();
			int AcquireRead(bool block = true);
			int AcquireWrite(bool block = true);
			int Release();
		private:
			pthread_rwlock_t rwmutex_;
	};
}


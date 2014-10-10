#include "yt/util/rw_mutex.h"

namespace yt
{
	ThreadRWMutex::ThreadRWMutex()
	{
		pthread_rwlock_init(&rwmutex_, NULL);
	}
	ThreadRWMutex::~ThreadRWMutex()
	{
		pthread_rwlock_destroy(&rwmutex_);
	}
	int ThreadRWMutex::AcquireRead(bool block)
	{
		if ( block ) {
			return pthread_rwlock_rdlock(&rwmutex_);
		}
		else {
			return pthread_rwlock_tryrdlock(&rwmutex_);
		}
	}
	int ThreadRWMutex::AcquireWrite(bool block)
	{
		if ( block ) {
			return pthread_rwlock_wrlock(&rwmutex_);
		}
		else {
			return pthread_rwlock_trywrlock(&rwmutex_);
		}
	}	
	int ThreadRWMutex::Release()
	{
		return pthread_rwlock_unlock(&rwmutex_);
	}

}

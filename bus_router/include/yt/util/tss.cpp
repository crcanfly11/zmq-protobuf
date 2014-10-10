#include <yt/util/tss.h>
#include <exception>
#include <assert.h>
#include <stdio.h>
#include "yt/util/lockguard.h"

namespace yt
{
	class TSSInitFail : public std::exception
	{
	};
	class TSSSetSpecific : public std::exception
	{
	};
	TSS::TSS(CLEANUP_FUNC f)
	{
		int result = pthread_key_create(&key, f);
		if (result != 0)
			assert(0);
		//throw TSSInitFail();
	}
	TSS::~TSS()
	{
		pthread_key_delete(key);
	}
	void TSS::set(void* p)
	{
		int result = pthread_setspecific(key, p);
		if (result != 0)
			assert(0);
		//throw TSSSetSpecific();
	}
	void* TSS::get()
	{
		return pthread_getspecific(key);
	}

	/*TSS2::~TSS2()
	{
		m_rwmutex.AcquireWrite();
		LockGuard g(&m_rwmutex);
		hash_map<pthread_t,void*>::iterator it;

		if(m_cleanfunc)
		{
			for(it = m_map.begin();it != m_map.end();it++)
			{
				m_cleanfunc(it->second);
			}
		}
		m_map.clear();
	}
	void TSS2::set(void* p)
	{
		pthread_t tid = pthread_self();
		m_rwmutex.AcquireWrite();
		LockGuard g(&m_rwmutex);
		hash_map<pthread_t,void*>::iterator it = m_map.find(tid);
		m_map[tid] = p;
	}
	void* TSS2::get()
	{
		pthread_t tid = pthread_self();
		m_rwmutex.AcquireRead();
		LockGuard g(&m_rwmutex);
		hash_map<pthread_t,void*>::iterator it = m_map.find(tid);
		if(it == m_map.end())
		{
			return NULL;
		}
		return it->second;
	}
	*/
}


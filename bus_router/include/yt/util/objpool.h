#pragma once

#include "yt/util/freesizememallocator.h"
#include "yt/util/lockguard.h"
#include "yt/log/stacktrace.h"
#include <list>

using namespace std;

//已经不用了，用tcmalloc取代

namespace yt
{
	class DestroyCallback
	{
		public:
			virtual void DestroyT(void *t) = 0;
	};
	class Destroyable
	{
		public:
			Destroyable() : m_pDcb(NULL){}
			void SetDcb(DestroyCallback *pDcb){
				m_pDcb = pDcb;
			}
			virtual void DestroyMe(){
				if(m_pDcb)
					m_pDcb->DestroyT(this);
			}
		private:
			DestroyCallback *m_pDcb;
	};
	template<class T>
		class ObjPool : public FreeSizeMemAllocator , public DestroyCallback
		{
			public:
				ObjPool(Mutex *pMutex = NullMutex::Instance(),size_t boundsize = BOUNDSIZE,size_t flmaxsize = MAXSIZE) : FreeSizeMemAllocator(pMutex,boundsize,flmaxsize){}
				~ObjPool()
				{
					//m_pMutex->Acquire();
					//LockGuard g(m_pMutex);
					typename list<T*>::iterator it;
					T *t;
					for(it = m_objlist.begin();it != m_objlist.end();it++)
					{
						t = *it;
						t->~T();//不但是调用自己的析构，还调用基类的，还析构了成员函数！
						//delete *it;
					}
					m_objlist.clear();
				}
				void Destroy(T *t)
				{
					m_pMutex->Acquire();
					LockGuard g(m_pMutex);
					m_objlist.push_back(t);
				}
				T* Create()
				{
					m_pMutex->Acquire();
					if(m_objlist.size() > 0)
					{
						T *t = *(m_objlist.begin());
						m_objlist.pop_front();
						m_pMutex->Release();
						return t;
					}
					//_AC_STACK_TRACE();
					m_pMutex->Release();
					void *p = Allocate(sizeof(T));
					if(!p)
						return NULL;
					return new(p) T;
				}
				void DestroyT(void *t)
				{
					Destroy((T*)t);
				}
			private:
				list<T*> m_objlist;
		};
	template<class T>
		class ObjGuard
		{
			public:
				ObjGuard(ObjPool<T> *pool,T *t) : m_pool(pool),m_t(t){}
				~ObjGuard(){
					m_pool->Destroy(m_t);
				}
			private:
				ObjPool<T> *m_pool;
				T *m_t;
		};
}


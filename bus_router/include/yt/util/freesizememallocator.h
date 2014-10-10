#pragma once

#include <map>
#include <list>
#include <vector>
#include "yt/util/mutex.h"
#include "yt/util/non_copyable.h"
#include "yt/util/tss.h"

using namespace std;

/*
里面的功能都已经不用了,用tcmalloc取代了
*/
namespace yt
{
	class FreeSizeMemAllocatorBase
	{
		protected:
			static const size_t BOUNDSIZE = 32;
			static const size_t MAXSIZE = 4096;
			static const size_t PAGESIZE = 4096;
			struct freelist
			{
				freelist *next;
			};
		public:
			FreeSizeMemAllocatorBase(Mutex *pMutex = NullMutex::Instance(),size_t boundsize = BOUNDSIZE,size_t flmaxsize = MAXSIZE) : m_pMutex(pMutex),m_boundsize(boundsize),m_flmaxsize(flmaxsize){}
			virtual ~FreeSizeMemAllocatorBase(){}
			void ProcessMemory(char *p,char * const end,size_t unitsize);
			static size_t GetFitSize(size_t size,size_t sizeclass);
			virtual void* Allocate(size_t size);
			virtual void Deallocate(void* pData,size_t size);
		private:
			virtual void* GetMemory(size_t size) = 0;
		protected:
			map<size_t,freelist*> m_freelistmap;
			Mutex *m_pMutex;
			size_t m_boundsize;
			size_t m_flmaxsize;
	};
	class FreeSizeMemAllocator : public FreeSizeMemAllocatorBase
	{
		public:
			FreeSizeMemAllocator(Mutex *pMutex = NullMutex::Instance(),size_t boundsize = BOUNDSIZE,size_t flmaxsize = MAXSIZE) : FreeSizeMemAllocatorBase(pMutex,boundsize,flmaxsize){}
			virtual ~FreeSizeMemAllocator();
		private:
			virtual void* GetMemory(size_t size);
			vector<void*> m_memlist;
	};
	class FSMPoolableBase
	{
		public:
			virtual ~FSMPoolableBase(){}//必须要有虚拟析构函数，否则new和delete的size会不对
	};
	class FSMPoolableThread : public FSMPoolableBase//允许多线程创建这个类
	{
		public:
			void* operator new(size_t size,const std::nothrow_t&) throw();
			void operator delete(void *p,size_t size);
		protected:
			static FreeSizeMemAllocator* GetAllocator();
		private:
			static void Clean_TSS(void *p);
			static /*TSS2*/TSS g_tss;
	};
}

#include "yt/util/freesizememallocator.h"
#include "yt/util/lockguard.h"
#include "yt/log/stacktrace.h"

namespace yt
{	
	size_t FreeSizeMemAllocatorBase::GetFitSize(size_t size,size_t sizeclass)
	{
		double f = (double)size / (double)sizeclass;
		int i = (int)f;
		
		return (i + 1) * sizeclass;
	}
	void FreeSizeMemAllocatorBase::ProcessMemory(char *p,char * const end,size_t unitsize)
	{
		map<size_t,freelist*>::iterator it;
		it = m_freelistmap.find(unitsize);
		if(it == m_freelistmap.end())
		{
			m_freelistmap.insert(make_pair(unitsize,(freelist*)NULL));
			it = m_freelistmap.find(unitsize);
		}
		while(p + unitsize <= end)
		{
			freelist *head = (freelist*)p; 
			head->next = it->second;//释放到队头
			it->second = head;
			p += unitsize;
		}
	}
	void* FreeSizeMemAllocatorBase::Allocate(size_t s)
	{
		m_pMutex->Acquire();
		LockGuard g(m_pMutex);

		if(s <= 0)
			return NULL;

		size_t size = GetFitSize(s,m_boundsize);
		
		if(size > m_flmaxsize)//比m_flmaxsize大，就直接malloc
		{
			void *p = malloc(size);
			return p;
		}

		size_t allocatesize = size <= PAGESIZE ? PAGESIZE : size;
		//printf("allocatesize = %d\n",allocatesize);

		map<size_t,freelist*>::iterator it;
		it = m_freelistmap.find(size);
		if(it == m_freelistmap.end())//没有这个级别的freelist
		{
			//void *p = GetMemory(size);
			void *p = GetMemory(allocatesize);
			if(p && allocatesize == PAGESIZE)
				ProcessMemory( (char*)p + size,(char*)p + PAGESIZE,size);

			return p;
		}
		else
		{
			if(it->second == NULL)
			{
				//void *p = GetMemory(size);
				void *p = GetMemory(allocatesize);
				if(p && allocatesize == PAGESIZE)
					ProcessMemory( (char*)p + size,(char*)p + PAGESIZE,size);

				return p;
			}
			else
			{
				void *p = it->second;
				it->second = it->second->next;
				return p;
			}
		}
		return NULL;
	}
	void FreeSizeMemAllocatorBase::Deallocate(void* pData,size_t size)
	{
		m_pMutex->Acquire();
		LockGuard g(m_pMutex);
		
		if(pData == NULL)
			return;

		size_t s = GetFitSize(size,m_boundsize);
		
		if(m_flmaxsize != 0 && s > m_flmaxsize)//比m_flmaxsize大，就直接free
		{
			free(pData);
			return;
		}

		freelist* head = (freelist*)pData;

		map<size_t,freelist*>::iterator it;
		it = m_freelistmap.find(s);
		if(it == m_freelistmap.end())
		{
			head->next = NULL;
			m_freelistmap.insert(make_pair(s,head));//新的连表
		}
		else
		{
			head->next = it->second;//释放到队头
			it->second = head;
		}
	}
	/*void FreeSizeMemAllocatorBase::GetInfo(char *info)
	{
		m_pMutex->Acquire();
		LockGuard g(m_pMutex);

		info[0] = 0;
		sprintf(info,"freesizemempool size = %d\n",(int)m_freelistmap.size());
		freelist *p;
		map<size_t,freelist*>::iterator it;
		for(it = m_freelistmap.begin();it != m_freelistmap.end();it++)
		{
			p = it->second;
			int s = 0;
			while(p)
			{
				s++;
				p = p->next;
			}
			char str[32];
			sprintf(str,"class = %d,count = %d\n",(int)it->first,s);
			strcat(info,str);
		}
		return info;
	}*/
	FreeSizeMemAllocator::~FreeSizeMemAllocator()
	{
		//m_pMutex->Acquire();
		//LockGuard g(m_pMutex);

		freelist *p,*p2;
		map<size_t,freelist*>::iterator it;
		for(it = m_freelistmap.begin();it != m_freelistmap.end();it++)
		{
			if(it->first > PAGESIZE)
			{
				p = it->second;
				while(p)
				{
					p2 = p->next;
					free(p);
					p = p2;
				}
			}
		}
		m_freelistmap.clear();

		for(size_t i = 0;i < m_memlist.size();i++)
		{
			free(m_memlist[i]);
		}
		m_memlist.clear();
	}
	void* FreeSizeMemAllocator::GetMemory(size_t size)
	{
		void *mem = malloc(size);
		if(mem)
		{
			if(size <= PAGESIZE)	m_memlist.push_back(mem);
		}
		return mem;
	}	

	/*TSS2*/TSS FSMPoolableThread::g_tss(Clean_TSS);
	
	void FSMPoolableThread::Clean_TSS(void *p)
	{
		//_AC_STACK_TRACE();
		FreeSizeMemAllocator *allocator = (FreeSizeMemAllocator*)p;
		delete allocator;
	}
	FreeSizeMemAllocator* FSMPoolableThread::GetAllocator()
	{
		FreeSizeMemAllocator *allocator = (FreeSizeMemAllocator*)g_tss.get();
		if(!allocator)
		{
			allocator = new(nothrow) FreeSizeMemAllocator;
			if(!allocator)
			{
				return NULL;
			}
			g_tss.set(allocator);
			return allocator;
		}
		return allocator;
	}
	void* FSMPoolableThread::operator new(size_t size,const std::nothrow_t&) throw()
	{
		FreeSizeMemAllocator *allocator = GetAllocator();
		if(!allocator){
			return NULL;
		}
		return allocator->Allocate(size);
	}
	void FSMPoolableThread::operator delete(void* p,size_t size)
	{
		FreeSizeMemAllocator *allocator = GetAllocator();
		if(!allocator){
			return;
		}
		return allocator->Deallocate(p,size);
	}
}

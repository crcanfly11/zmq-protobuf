#include <yt/util/allocator.h>
#include <new>
#include <sys/types.h>
#include <stdlib.h>
#include <yt/log/stacktrace.h>
#include <yt/util/mutex.h>

namespace yt
{
	void * MallocAllocator::Allocate(size_t iSize, size_t * pRealSize)
	{
		if ( pRealSize != NULL ) {
			*pRealSize = iSize;
		}
		return malloc(iSize);
	}

	void MallocAllocator::Deallocate(void * pData)
	{
		return free(pData);
	}
	MallocAllocator * MallocAllocator::Instance()
	{
		static MallocAllocator oMallocAllocator;
		return &oMallocAllocator;
	}
	void * NewAllocator::Allocate(size_t iSize, size_t * pRealSize)
	{
		if ( pRealSize != NULL ) {
			*pRealSize = iSize;
		}	
		return ::operator new(iSize, ::std::nothrow_t());
	}
	void NewAllocator::Deallocate(void * pData)
	{
		return ::operator delete(pData, ::std::nothrow_t());
	}
	NewAllocator * NewAllocator::Instance()
	{
		static NewAllocator oNewAllocator;
		return &oNewAllocator;
	}
	Allocator * Allocator::Instance()
	{
		return NewAllocator::Instance();
	}
}


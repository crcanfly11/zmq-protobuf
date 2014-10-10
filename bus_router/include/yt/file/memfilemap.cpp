#include <stdlib.h>
#include <math.h>
#include "yt/file/memfilemap.h"
#include "yt/file/file.h"

namespace yt
{
	bool MemFileMapping::CreateMappingFile(const char* pathname,size_t length)
	{
		if(File::FileExists(pathname))
			return true;
			
		File f(pathname);
		if(!f.Open(O_RDWR | O_CREAT))
			return false;
		char buf[4096];
		size_t buflen = sizeof(buf);
		memset(buf, 0, sizeof(buf));
		size_t remain = length;
		while(remain)
		{
			//size_t len = min(remain, buflen);
			size_t len = (remain < buflen) ? remain : buflen;
			if(!f.Write(buf,len))
				return false;
			remain -= len;
		}

		f.Close();
		return true;
	}
	bool MemFileMapping::Map(int fd,off_t offset,size_t length)
	{
		m_fd = fd;
		m_offset = offset;
		m_length = length;

		if( (m_start = mmap(0,m_length,PROT_READ | PROT_WRITE,MAP_SHARED,m_fd,m_offset)) == MAP_FAILED)
			return false;
		return true;
	}

	void MemFileMapping::UnMap()
	{
		if(!m_start)
			return;
		Sync();
		munmap(m_start,m_length);
	}
	void MemFileMapping::UnMap2()
	{
		if(!m_start)
			return;
		munmap(m_start,m_length);
	}
	void MemFileMapping::Sync()
	{
		int PageSize = sysconf(_SC_PAGE_SIZE);
		msync(m_start,PageSize,MS_SYNC);
	}
}

#pragma once

#include <stdio.h>
#include <sys/mman.h>

namespace yt
{
	class MemFileMapping
	{
		public:
			//MemFileMapping(size_t length,int fd,off_t offset) : m_start(NULL),m_length(length),m_fd(fd),m_offset(offset){}
			MemFileMapping() : m_start(NULL){}
			static bool CreateMappingFile(const char* pathname,size_t length);
			bool Map(int fd,off_t offset,size_t length);
			void UnMap();
			void UnMap2();
			void Sync();
			void* GetStart() const {return m_start;}
		private:
			void *m_start;
			size_t m_length;
			int m_fd;
			off_t m_offset;
	};
}

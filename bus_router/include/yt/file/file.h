#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

namespace yt
{
	class File
	{
		static const mode_t MODE = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
		public:
			File() : m_fd(-1){}
			File(const char *pathname) : m_fd(-1)
			{
				strncpy(m_pathname,pathname,sizeof(m_pathname));
			}
			inline int GetFD() const{
				return m_fd;
			}
			inline void SetFD(int fd){
				m_fd = fd;
			}
			bool Open(const char* pathname,int flags,mode_t mode = MODE);
			bool Open(int flags,mode_t mode = MODE);
			void Close();
			inline int Read(void *buf,size_t count) const
			{
				if(m_fd == -1)
					return -2;
				return read(m_fd,buf,count);
			}
			inline int Write(const void *buf,size_t count) const
			{
				if(m_fd == -1)
					return -2;
				return write(m_fd,buf,count);
			}
			off_t Seek(int whence,off_t offset) const
			{
				if(m_fd == -1)
					return -2;
				return lseek(m_fd,offset,whence);
			}
			off_t Teel() const
			{
				if(m_fd == -1)
					return -2;
				return Seek(SEEK_CUR,0);
			}
			static bool FileExists(const char* pathname){
				if(access(pathname,F_OK) == 0)
					return true;
				return false;
			}
			static size_t GetFileSize(const char* pathname);
		private:
			int m_fd;
			char m_pathname[256];
	};
	class FileGuard
	{
		public:
			FileGuard(File *f) : m_f(f){}
			~FileGuard(){m_f->Close();}
		private:
			File *m_f;
	};
}

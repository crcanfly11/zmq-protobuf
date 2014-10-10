#include <errno.h>
#include "yt/file/file.h"

namespace yt
{
	bool File::Open(const char* pathname,int flags,mode_t mode)
	{
		if(m_fd != -1)
			Close();
		strncpy(m_pathname,pathname,sizeof(m_pathname));
		return Open(flags,mode);
	}
	bool File::Open(int flags,mode_t mode)
	{
		if(m_fd != -1)
			Close();
		if( (m_fd = open(m_pathname,flags,mode)) == -1){
			return false;
		}
		return true;
	}
	void File::Close()
	{
		if(m_fd == -1)
			return;
		close(m_fd);
		m_fd = -1;
	}
	/*bool File::ReadAll(void *buf,size_t count)
	{
		int ret;
		int left = count;
		char *ptr = (char*)buf;

		int i = 0;
		while(left > 0 && i++ < 100)
		{
			ret = Read(ptr,left);
			if(ret == -2){
				return false;
			}
			else if(ret == -1)
			{
				if(errno == EINTR)//信号中断
					continue;
				else
					return false;
			}
			left = left - ret;
			if(left == 0)
				return true;
			ptr += ret;
		}
		return false;
	}
	bool File::WriteAll(void *buf,size_t count)
	{
		int ret;
		int left = count;
		char *ptr = (char*)buf;

		int i = 0;
		while(left > 0 && i++ < 100)
		{
			ret = Write(ptr,left);
			if(ret == -2){
				return false;
			}
			else if(ret == -1)
			{
				if(errno == EINTR)//信号中断
					continue;
				else
					return false;
			}
			left = left - ret;
			if(left == 0)
				return true;
			ptr += ret;
		}
		return false;
	}*/
	size_t File::GetFileSize(const char* pathname)
	{
		struct stat buf;
		if(lstat(pathname,&buf) == -1){
			return 0;
		}
		return buf.st_size;
	}
}

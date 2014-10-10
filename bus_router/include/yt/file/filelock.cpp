#include <sys/file.h>
#include <errno.h>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <string.h>
#include "yt/util/lockguard.h"
#include "yt/file/filelock.h"

namespace yt
{
	bool FileRecLock::AcquireRead(bool block)
	{
		struct flock    lock;

		lock.l_type = F_RDLCK;     /* F_RDLCK, F_WRLCK, F_UNLCK */
		lock.l_start = m_offset;  /* byte offset, relative to l_whence */
		lock.l_whence = m_whence; /* SEEK_SET, SEEK_CUR, SEEK_END */
		lock.l_len = m_len;       /* #bytes (0 means to EOF) */

		if(block)
		{
			int i = 0;
			while(i++ < 100)
			{
				if(fcntl(m_fd,F_SETLKW,&lock) == 0)
					return true;
				else
				{
					if(errno == EINTR)
						continue;
				}
			}
			return false;
		}
		if(fcntl(m_fd,F_SETLK,&lock) == 0)
			return true;
	
		return false;
	}
	bool FileRecLock::AcquireWrite(bool block)
	{
		struct flock    lock;

		lock.l_type = F_WRLCK;     /* F_RDLCK, F_WRLCK, F_UNLCK */
		lock.l_start = m_offset;  /* byte offset, relative to l_whence */
		lock.l_whence = m_whence; /* SEEK_SET, SEEK_CUR, SEEK_END */
		lock.l_len = m_len;       /* #bytes (0 means to EOF) */

		if(block)
		{
			int i = 0;
			while(i++ < 100)
			{
				if(fcntl(m_fd,F_SETLKW,&lock) == 0)
					return true;
				else
				{
					if(errno == EINTR)
						continue;
				}
			}
			return false;
		}
		if(fcntl(m_fd,F_SETLK,&lock) == 0)
			return true;
	
		return false;
	}
	int FileRecLock::Release()
	{
		struct flock    lock;

		lock.l_type = F_UNLCK;     /* F_RDLCK, F_WRLCK, F_UNLCK */
		lock.l_start = m_offset;  /* byte offset, relative to l_whence */
		lock.l_whence = m_whence; /* SEEK_SET, SEEK_CUR, SEEK_END */
		lock.l_len = m_len;       /* #bytes (0 means to EOF) */

		fcntl(m_fd,F_SETLK,&lock);
		return 0;
	}
	bool FileAllLock::AcquireRead(bool block)
	{	
		int op = LOCK_SH;
		if(!block)
			op = op | LOCK_NB;
		
		int i = 0;
		while(i++ < 100)
		{
			if(flock(m_fd,op) == 0)
			{
				return true;
			}
			else
			{
				if(errno == EINTR)
				{
					//AC_ERROR("EINTR");
					continue;
				}
				else
					return false;
			}
		}
		return false;
	}
	bool FileAllLock::AcquireWrite(bool block)
	{
		int op = LOCK_EX;
		if(!block)
			op = op | LOCK_NB;

		int i = 0;
		while(i++ < 100)
		{
			if(flock(m_fd,op) == 0)
			{
				return true;
			}
			else
			{
				if(errno == EINTR)
				{
					continue;
				}
				else
					return false;
			}
		}
		return false;
	}
	int FileAllLock::Release()
	{
		return flock(m_fd,LOCK_UN);
	}
	FileThreadRecLockPool* FileThreadRecLockPool::Instance()
	{
		static FileThreadRecLockPool pool;
		return &pool;
	}
	FileThreadRecLockPool::~FileThreadRecLockPool()
	{
		FILEMAP::iterator it;
		for(it = m_filemap.begin();it != m_filemap.end();it++)
		{
			delete it->second;
		}
		m_filemap.clear();
	}
	void FileThreadRecLockPool::Release(const char* filename,LOCKTYPE type,off_t off,off_t len,pthread_t tid)
	{
		m_mutex.Acquire();
		LockGuard g(&m_mutex);

		FILEMAP::iterator it = m_filemap.find(filename);
		if(it == m_filemap.end())
			assert(0);

		AREALIST *arealist = it->second;
		AREALIST::iterator it2;
		for(it2 = arealist->begin();it2 != arealist->end();it2++)
		{
			if(it2->type == type && it2->off == off && it2->len == len && it2->tid == tid)
			{
				arealist->erase(it2);
				return;
			}
		}
	}
	int FileThreadRecLockPool::Acquire(const char* filename,LOCKTYPE type,off_t off,off_t len,pthread_t tid)
	{
		m_mutex.Acquire();
		LockGuard g(&m_mutex);

		AREALIST *arealist;
		FILEMAP::iterator it2 = m_filemap.find(filename);
		if(it2 == m_filemap.end())
		{
			arealist = new AREALIST;
			m_filemap.insert(make_pair(filename,arealist));
		}
		else
			arealist = it2->second;

		AREALIST::iterator it;
		for(it = arealist->begin();it != arealist->end();it++)
		{
			if(type == WRITE)
			{
				if(IsIntersect(off,len,it->off,it->len))
				{
					if(tid == it->tid)
					{
						assert(0);
					}
					else
					{
						return -1;
					}
				}
			}
			else
			{
				if(it->type == WRITE)
				{
					if(IsIntersect(off,len,it->off,it->len))//区域有重叠
					{
						if(tid == it->tid)//自己线程
						{
							assert(0);
						}
						else
						{
							return -1;
						}
					}
				}
			}
		}
		AreaInfo ai = {off,len,type,tid};
		arealist->push_back(ai);
		return 0;
	}
	bool FileThreadRecLockPool::IsIntersect(off_t off1,off_t len1,off_t off2,off_t len2)
	{
		vector<off_t> v;
		v.push_back(off1);
		v.push_back(off1 + len1);
		v.push_back(off2);
		v.push_back(off2 + len2);

		sort(v.begin(),v.end());

		off_t distant = v[3] - v[0];
		if(distant < (len1 + len2))
			return true;
		return false;
	}
	FileThreadRecLock::FileThreadRecLock(const char* filename,off_t off,off_t len) : m_off(off),m_len(len)
	{
		strncpy(m_filename,filename,sizeof(m_filename));
	}
	void FileThreadRecLock::AcquireRead()
	{
		m_locktype = READ;
		m_tid = pthread_self();
		while(1)
		{
			if(FileThreadRecLockPool::Instance()->Acquire(m_filename,m_locktype,m_off,m_len,m_tid) != 0)
				usleep(1);
			else
				return;
		}
	}
	void FileThreadRecLock::AcquireWrite()
	{
		m_locktype = WRITE;
		m_tid = pthread_self();
		while(1)
		{
			if(FileThreadRecLockPool::Instance()->Acquire(m_filename,m_locktype,m_off,m_len,m_tid) != 0)
				usleep(1);
			else
				return;
		}
	}
	int FileThreadRecLock::Release()
	{
		FileThreadRecLockPool::Instance()->Release(m_filename,m_locktype,m_off,m_len,m_tid);
		return 0;
	}
}


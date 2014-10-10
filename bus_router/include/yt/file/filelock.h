#pragma once

#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "yt/util/non_copyable.h"
#include "yt/util/lockable.h"
#include "yt/util/mutex.h"
#include "yt/util/rw_mutex.h"
#include "yt/util/lockguard.h"
#include <map>
#include <list>
#include <vector>
#include <string>

using namespace std;

namespace yt
{		
	class FileLock : public Lockable
	{
		public:
			FileLock(){}
			FileLock(int fd) : m_fd(fd){}
			virtual ~FileLock(){}
			virtual bool AcquireRead(bool block = true) = 0;
			virtual bool AcquireWrite(bool block = true) = 0;
			void SetFD(int fd){m_fd = fd;}
		protected:
			int m_fd;
	};
	class FileRecLock : public FileLock
	{
		public:
			FileRecLock(int fd,int whence,off_t offset,off_t len) : FileLock(fd),m_whence(whence),m_offset(offset),m_len(len){}
			bool AcquireRead(bool block = true);
			bool AcquireWrite(bool block = true);
			int Release();
		private:
			int m_whence;
			off_t m_offset;
			off_t m_len;
	};
	class FileAllLock : public FileLock
	{
		public:
			FileAllLock(){}
			FileAllLock(int fd) : FileLock(fd){}
			bool AcquireRead(bool block = true);
			bool AcquireWrite(bool block = true);
			int Release();
	};
	enum LOCKTYPE
	{
		READ = 0x01,
		WRITE,
	};
	class FileThreadRecLockPool : public NonCopyable
	{
		struct AreaInfo
		{
			off_t off;
			off_t len;
			LOCKTYPE type;
			pthread_t tid;
		};
		typedef list<AreaInfo> AREALIST;
		typedef map<string, AREALIST*> FILEMAP;
		public:
			static FileThreadRecLockPool* Instance();
			~FileThreadRecLockPool();
			void Release(const char* filename,LOCKTYPE type,off_t off,off_t len,pthread_t tid);
			int Acquire(const char* filename,LOCKTYPE type,off_t off,off_t len,pthread_t tid);
			bool IsIntersect(off_t off1,off_t len1,off_t off2,off_t len2);
		private:
			FileThreadRecLockPool(){}
			FILEMAP m_filemap;
			ThreadMutex m_mutex;
	};
	class FileThreadRecLock : public Lockable
	{
		public:
			FileThreadRecLock(const char* filename,off_t off,off_t len);
			void AcquireRead();
			void AcquireWrite();
			int Release();
		private:
			char m_filename[256];
			LOCKTYPE m_locktype;
			off_t m_off;
			off_t m_len;
			pthread_t m_tid;
	};
}


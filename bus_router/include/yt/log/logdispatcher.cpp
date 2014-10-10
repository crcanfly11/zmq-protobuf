#include "yt/util/rw_mutex.h"
#include "yt/log/logdispatcher.h"
#include "yt/log/logpriority.h"
#include "yt/log/logger.h"
#include "yt/log/nulllogger.h"
#include "yt/util/lockguard.h"

namespace yt
{
	LogDispatcher * LogDispatcher::Instance()
	{
		static LogDispatcher oLogDispatcher;
		return &oLogDispatcher;
	}
	LogDispatcher::LogDispatcher()
		: pDefaultLogger_(NullLogger::Instance())
		{
		}
	void LogDispatcher::SetDefaultLogger(Logger * pDefaultLogger)
	{
		mutex_.AcquireWrite();
		LockGuard g(&mutex_);

		pDefaultLogger_ = (pDefaultLogger == 0 ? NullLogger::Instance() : pDefaultLogger);
	}
	void LogDispatcher::SetLogger(LogPriority iPriority, Logger * pNewLogger)
	{
		mutex_.AcquireWrite();
		LockGuard g(&mutex_);

		AtomicSetLogger(iPriority, pNewLogger);
	}
	Logger * LogDispatcher::GetLogger(LogPriority iPriority)
	{
		mutex_.AcquireRead();
		LockGuard g(&mutex_);

		LoggerMap::iterator pos = mLoggerMap_.find(iPriority);
		return pos == mLoggerMap_.end() ? pDefaultLogger_ : pos->second;
	}
	void LogDispatcher::AtomicSetLogger(LogPriority iPriority, Logger * pNewLogger)
	{
		mLoggerMap_[iPriority] = (pNewLogger == 0 ? NullLogger::Instance() : pNewLogger); 
	}
}


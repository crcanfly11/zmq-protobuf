#pragma once
 
#include <map>
#include <string>

#include "yt/util/rw_mutex.h"
#include "yt/util/non_copyable.h"
#include "yt/log/logpriority.h"
#include "yt/log/logger.h"

namespace yt
{
	class LogDispatcher : public NonCopyable
	{
		public:
			static LogDispatcher * Instance();
		public:
			void SetDefaultLogger(Logger * pDefaultLogger);
			void SetLogger(LogPriority iPriority, Logger * pNewLogger);
			Logger * GetLogger(LogPriority iPriority);
			inline int Log(LogPriority iPriority, const std::string &s)
			{
				return GetLogger(iPriority)->Log(iPriority, s);
			}
		protected:
			LogDispatcher();
			void AtomicSetLogger(LogPriority iPriority, Logger * pNewLogger);
		private:
			typedef std::map<LogPriority, Logger *> LoggerMap;
			LoggerMap mLoggerMap_;
			Logger * pDefaultLogger_;
			ThreadRWMutex mutex_;
	};
}


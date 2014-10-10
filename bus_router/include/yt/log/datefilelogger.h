#pragma once

#include <sys/types.h>
#include <string>

#include "yt/log/logger.h"
#include "yt/util/mutex.h"

//! \namespace OssBase
namespace yt
{

	/*! \class DateFileLogger
	 * \brief 日期文件日志处理类
	 */
	class DateFileLogger : public Logger
	{
		public:

			/*! \brief 构造函数
			 * \param[in] szLogFile 日志文件路径 ( 请尽量使用绝对路径 )
			 */
			DateFileLogger(const std::string & sLogFile, const std::string & sLogFormat=/*Logger::*/DEFAULT_FORMAT);
			virtual ~DateFileLogger();


			/*! \brief 实现了Log方法，将sLogInfo写入时间文件
			 * \param[in] iLogPriority 日志级别
			 * \param[in] sLogMsg 日志信息
			 * \return 0:成功 非0:失败
			 */
			virtual int Log(LogPriority iLogPriority,const std::string & sLogMsg);

		protected:

			//! \brief 得到日志文件名
			//! \return 带日期的文件名
			std::string GetLogFileName();

		private:

			//! \brief 日志路径
			std::string sLogFile_;

			ThreadMutex mutex_;
			 
			int m_fd;
			struct tm cur_time;

	};

}

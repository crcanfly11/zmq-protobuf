#include <iostream>
#include <string>

#include "yt/util/mutex.h"
#include "yt/log/ostreamlogger.h"
#include "yt/log/logger.h"
#include "yt/util/lockguard.h"

namespace yt
{

OStreamLogger::OStreamLogger(std::ostream & stream, const std::string & sLogFormat)
	: Logger(sLogFormat), stream_(stream)
{
}

int OStreamLogger::Log(LogPriority ,const std::string & s)
{
	/*LockGuard lock(&mutex_);
	if ( lock.Lock(true) == 0 ) {
		stream_ << s << std::endl; 
	}
	lock.Unlock();

	return stream_? 0 : -1;*/
	mutex_.Acquire();
	LockGuard g(&mutex_);

	stream_ << s << std::flush;// << std::endl;
	return 0;
}

//OStreamLogger::OStreamLogger OStreamLogger::StdoutLogger(std::cout);
//OStreamLogger::OStreamLogger OStreamLogger::StderrLogger(std::cerr);

}


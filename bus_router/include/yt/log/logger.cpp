#include <iostream>

#include "yt/util/mutex.h"
#include "yt/log/logger.h"
#include "yt/log/nulllogger.h"
#include "yt/log/ostreamlogger.h"

bool g_writerunlog = false;
namespace yt
{
	//const std::string Logger::DEFAULT_FORMAT="[%D][%P] %m (%F:%L %f)";
}


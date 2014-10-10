#include "yt/network/eventhandler.h"
#include "yt/log/log.h"
#include <fcntl.h>

namespace yt
{
	int FDEventHandler::SetNonBlocking(bool blocking)
	{
		return SetNonBlocking(m_fd,blocking);
	}
	int FDEventHandler::SetNonBlocking(int fd,bool blocking)
	{
		int flags = fcntl(fd, F_GETFL, 0);
		if (flags == -1)
		{
			AC_ERROR("fcntl error");
			return -1;
		}

		if (blocking)
			flags |= O_NONBLOCK;
		else
			flags &= ~O_NONBLOCK;

		if (fcntl(fd, F_SETFL, flags) != 0)
		{
			AC_ERROR("fcntl error");
			return -1;
		}

		return 0;
	}
}


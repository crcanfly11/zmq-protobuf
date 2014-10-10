#include "yt/network/netproxynew.h"
#include "yt/log/log.h"
#include "yt/util/timeuse.h"

namespace yt
{
	bool NotifyEventHandler2::Init()
	{
		// socket
		if(socketpair(AF_UNIX, SOCK_STREAM, 0, m_fdpair) < 0)
		{
			AC_ERROR("socketpair error");
			return false;
		}

		// set non block(½ö¶Á)
		if(FDEventHandler::SetNonBlocking(m_fdpair[1],true) != 0)
		{
			AC_ERROR("set non blocing error");
			return false;
		}

		// set fd
		FDEventHandler::SetFD(m_fdpair[1]);			// ¶Á

		// register
		if(FDEventHandler::RegisterRead((struct timeval*)NULL) != 0)
		{
			AC_ERROR("register socket pair error");
			return false;
		}
		
		return true;
	}

	void NotifyEventHandler2::Close() 
	{
		close(m_fdpair[0]);
		FDEventHandler::Close();
	}
}

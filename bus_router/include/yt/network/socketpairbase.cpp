#include "socketpairbase.h"

#include <sys/socket.h>
#include <errno.h>
#include <sys/epoll.h>
#include <string.h>

#include "yt/log/log.h"

namespace yt
{
	void SocketPairBase::OnFDRead()
	{
		char buf[64];
		ssize_t len = recv(m_fd,buf,sizeof(buf),0);
		if( len == 0 )
		{
			AC_INFO("socketpair closed!");
			FDEventHandler::Close();
			return;
		}
		else if( len < 0 )
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
			{
				if(GetReactor()->IsETModel())
				{
					((EPETReactor *)GetReactor())->CancelFDEvent(m_fd, EPOLLIN);
				}
				return;
			}
			else
			{
				AC_ERROR("recv error, errdesc = %s", strerror(errno));
				FDEventHandler::Close();
				return;
			}
		}

		m_pDecoder->Process(this,buf,len);
	}
}

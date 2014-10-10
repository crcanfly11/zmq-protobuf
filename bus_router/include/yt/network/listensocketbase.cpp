#include "yt/network/listensocketbase.h"
#include "yt/log/log.h" 
#include <sys/types.h>  
#include <sys/socket.h> 
#include <sys/un.h>
#include <sys/stat.h> 
#include <sys/epoll.h>
#include <netinet/in.h> 
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>  
#include <errno.h>  
#include <stddef.h>  

namespace yt
{
	ListenSocketBase::ListenSocketBase(const char* host,int port,Reactor *pReactor,int clientreadtimeout) :
		FDEventHandler(pReactor),
		m_port(port),
		m_clientreadtimeout(clientreadtimeout)
	{
		strncpy(m_host,host,sizeof(m_host));
		if(m_clientreadtimeout > MAXCLIENTREADTIMEOUT)
			m_clientreadtimeout = MAXCLIENTREADTIMEOUT;
	}
	ListenSocketBase::ListenSocketBase(int fd,Reactor *pReactor,int clientreadtimeout) : FDEventHandler(pReactor),m_port(0),m_clientreadtimeout(clientreadtimeout)
	{
		if(m_clientreadtimeout > MAXCLIENTREADTIMEOUT)
			m_clientreadtimeout = MAXCLIENTREADTIMEOUT;
		SetFD(fd);
	}
	UnixListenSocketBase::UnixListenSocketBase(const char *path,Reactor *pReactor,int clientreadtimeout) :
		FDEventHandler(pReactor),
		m_clientreadtimeout(clientreadtimeout)
	{
		strncpy(m_path,path,sizeof(m_path));
	}
	UnixListenSocketBase::UnixListenSocketBase(int fd,Reactor *pReactor,int clientreadtimeout) :
		FDEventHandler(pReactor),
		m_clientreadtimeout(clientreadtimeout)
	{
		memset(m_path,0,sizeof(m_path));
		SetFD(fd);
	}
	int UnixListenSocketBase::Listen()
	{
		if(strlen(m_path) != 0)
		{
			struct sockaddr_un un;

			memset(&un, 0, sizeof(un));
			un.sun_family = AF_UNIX;
			strcpy(un.sun_path, m_path);
			m_fd = socket(AF_UNIX, SOCK_STREAM, 0);
			if (m_fd < 0) 
			{
				AC_ERROR("create socket error");
				return -1;
			}
			size_t size = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);
			if(::bind(m_fd, (struct sockaddr*)&un, size) != 0) 
			{
				AC_ERROR("bind error");
				close(m_fd);
				return -1;
			}
			if(::listen(m_fd,500) != 0)
			{
				AC_ERROR("listen error");	
				close(m_fd);
				return -1;
			}
		}
		if(RegisterRead((struct timeval*)NULL) != 0)
		{
			AC_ERROR("register read error");
			close(m_fd);
			return -1;
		}
		return 0;
	}
	int ListenSocketBase::Listen()
	{
		if(m_port != 0)
		{
			m_fd = socket(AF_INET,SOCK_STREAM,0);
			if(m_fd < 0)
			{
				AC_ERROR("create socket error");
				return -1;
			}
			sockaddr_in addr;
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_port = htons(m_port);
			addr.sin_addr.s_addr = inet_addr(m_host);

			int i = 1;
			if(setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&i, sizeof(int)) == -1)
			{
				AC_ERROR("setsocketopt error");
				return -1;
			}

			//set keepalive
			int keepalive = 1;  
			int keepidle = 300; 
			int keepinterval = 2;
			int keepcount = 3; 
			setsockopt(m_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive)); 
			setsockopt(m_fd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepidle , sizeof(keepidle)); 
			setsockopt(m_fd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepinterval , sizeof(keepinterval)); 
			setsockopt(m_fd, SOL_TCP, TCP_KEEPCNT, (void *)&keepcount , sizeof(keepcount)); 		
			
			if(FDEventHandler::SetNonBlocking(m_fd,true) != 0)
			{
				AC_ERROR("set nonblocking error");
				close(m_fd);
				return -1;
			}
			if (::bind(m_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
			{
				AC_ERROR("bind error");
				close(m_fd);
				return -1;
			}
			if(::listen(m_fd,500) != 0)
			{
				AC_ERROR("listen error");	
				close(m_fd);
				return -1;
			}
		}
		if(RegisterRead((struct timeval*)NULL) != 0)
		{
			AC_ERROR("register read error");
			close(m_fd);
			return -1;
		}
		return 0;
	}
	void UnixListenSocketBase::OnFDRead()
	{
		struct sockaddr_un  un;
		//struct stat         statbuf;

		socklen_t len = sizeof(un);
		int s = ::accept(m_fd,(struct sockaddr *)&un, &len);
		if (s == -1)
		{
			if(GetReactor()->IsETModel())
			{
				((EPETReactor *)GetReactor())->CancelFDEvent(m_fd, EPOLLIN);
				if(errno == EAGAIN || errno == EINTR)
					return;
			}
			AC_ERROR("accept error,errdesc = %s",strerror(errno));
			return;     /* often errno=EINTR, if signal caught */
		}

		/* obtain the client's uid from its calling address */
		len -= offsetof(struct sockaddr_un, sun_path); /* len of pathname */
		un.sun_path[len] = 0;           /* null terminate */

		unlink(un.sun_path);
		FDEventHandler::SetNonBlocking(s);
		OnAccept(s);
	}
	void ListenSocketBase::OnFDRead()
	{
		sockaddr_in clientaddr;
		socklen_t len = sizeof(clientaddr);

		int s = ::accept(m_fd, reinterpret_cast<sockaddr*>(&clientaddr), &len);
		if (s == -1)
		{
			if(GetReactor()->IsETModel())
			{
				((EPETReactor *)GetReactor())->CancelFDEvent(m_fd, EPOLLIN);
				if(errno == EAGAIN || errno == EINTR)
					return;
			}
			AC_ERROR("accept error,errdesc = %s",strerror(errno));
			return;
		}
		FDEventHandler::SetNonBlocking(s);	//ÉèÖÃ·Ç×èÈû
		OnAccept(s);
	}
	int UdpListenSocketBase::Listen()
	{
		struct sockaddr_in servaddr;
		m_fd = (int)socket(AF_INET, SOCK_DGRAM, 0);
		memset(&servaddr,0,sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = inet_addr(m_host.c_str());
		//servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		servaddr.sin_port = htons(m_port);

		int i = 1;
		if(setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&i, sizeof(int)) == -1)
		{
			AC_ERROR("setsockopt error");
			return -1;
		}
		if(bind(m_fd,(struct sockaddr *)&servaddr,sizeof(servaddr)) != 0)
		{
			AC_ERROR("bind error");
			return -1;
		}
		
		if(RegisterRead((struct timeval*)NULL) != 0)
		{
			AC_ERROR("register read error");
			return -1;
		}
		return 0;
	}

	void UdpListenSocketBase2::OnFDRead()
	{
		int n;
		socklen_t len;
		struct sockaddr_in pcliaddr;
		char inbuf[4096];// = {0};
		char outbuf[4096];// = {0};
		size_t outbuflen = 0;
		len = sizeof(pcliaddr);

		n = recvfrom(m_fd, inbuf, sizeof(inbuf), 0, (struct sockaddr *)&pcliaddr, &len);
		AC_INFO("recv n=%d,message=%s",n,inbuf);
		
		if(ProcessData(inbuf,(size_t)n,outbuf,outbuflen,(struct sockaddr *)&pcliaddr) == 0)
		{
			if(outbuflen > 0)
				sendto(m_fd,outbuf,outbuflen,0,(struct sockaddr *)&pcliaddr,len);
		}
	}
}

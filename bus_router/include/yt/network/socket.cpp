#include <sys/un.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include "yt/network/socket.h"
#include "yt/log/log.h"
#include <net/if.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include "yt/util/scope_guard.h"


namespace yt 
{
	int Socket::Create(int domain, int type, int protocol)
	{
		handle = socket(domain, type, protocol);
		if (handle == -1)
			return -1;
		return 0;
	}

	int Socket::ConnectUnixSocket(const char* path, unsigned int usec_timeout)
	{
		sockaddr_un addr;
		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		strncpy(addr.sun_path, path, sizeof(addr.sun_path));
		return Connect(reinterpret_cast<sockaddr*>(&addr), SUN_LEN(&addr), usec_timeout);
	}

	int Socket::Connect(const char* host, unsigned short port, unsigned int usec_timeout)
	{
		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		if (host)
		{
			addr.sin_addr.s_addr = inet_addr(host);
			if (addr.sin_addr.s_addr == (in_addr_t)-1)
			{
				return -1;
			}
		}

		return Connect(reinterpret_cast<sockaddr*>(&addr), sizeof(addr), usec_timeout);
	}

	int Socket::Connect(const sockaddr* addr, size_t addrlen, unsigned int usec_timeout)
	{
		if (usec_timeout == 0)
		{
			return ::connect(handle, addr, addrlen);
		}
		else
		{
			bool blocking;
			if (GetBlocking(blocking) == -1)
				return -1;

			if (blocking)
			{
				if (SetBlocking(false) == -1)
					return -1;
			}

			int ret = ::connect(handle, addr, addrlen);
			if (ret == -1)
			{
				if (errno == EINPROGRESS)
				{
					/*
					   timeval timeout;
					   memset(&timeout, 0, sizeof(timeout));
					   timeout.tv_usec = usec_timeout;

					   fd_set set;
					   FD_ZERO(&set);
					   FD_SET(handle, &set);

					   int ret = select(handle + 1, NULL, &set, NULL, &timeout);
					   if (ret <= 0)
					   {
					   return -1;
					   }
					   else
					   {
					   int error = -1;
					   int len = sizeof(error);
					   getsockopt(handle, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len);
					   if (error != 0)
					   return -1;
					   }
					 */
					int epfd = epoll_create(2);
					AC_ON_BLOCK_EXIT(close, epfd);

					int op = EPOLL_CTL_ADD;
					__uint32_t evtype = EPOLLOUT;

					struct epoll_event ev; 
					ev.data.fd = handle;
					ev.events = evtype;

					int ret = epoll_ctl(epfd,op,handle,&ev);
					if(ret < 0)
						return -1; 

					struct epoll_event events[4];
					int nfds = epoll_wait(epfd,events,4,usec_timeout);
					if(nfds < 0)
						return -1; 

					int sockfd;
					for(int i = 0;i < nfds;++i)
					{   
						if(events[i].events & EPOLLOUT)
						{   
							if( (sockfd = events[i].data.fd) < 0)
								return -1; 
							int error = -1; 
							int len = sizeof(error);
							getsockopt(handle, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len);
							if (error != 0)
								return -1; 
						}   
						else
						{   
							return -1; 
						}   
					}   
				}
				else
				{
					return -1;
				}
			}

			if (blocking)
			{
				if (SetBlocking(true) == -1)
					return -1;
			}

			return 0;
		}
	}

	int Socket::BindUnixSocket(const char* path)
	{
		sockaddr_un addr;
		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		strncpy(addr.sun_path, path, sizeof(addr.sun_path));
		return Bind(reinterpret_cast<sockaddr*>(&addr), SUN_LEN(&addr));
	}

	int Socket::Bind(const char* host, unsigned short port)
	{
		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		if (host)
		{
			addr.sin_addr.s_addr = inet_addr(host);
			if (addr.sin_addr.s_addr == (in_addr_t)-1)
				return -1;
		}

		return Bind(reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
	}

	int Socket::Bind(const sockaddr* addr, size_t addrlen)
	{
		if (handle == -1)
			return -1;

		int i = 1;
		if(setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, (void*)&i, sizeof(int)) == -1)
		{
			return -1;
		}

		if (::bind(handle, addr, addrlen) != 0)
		{
			return -1;
		}

		return 0;
	}

	int Socket::Listen(unsigned int backlog)
	{
		return ::listen(handle, backlog);
	}

	int Socket::Accept(Socket& client)
	{
		sockaddr_in clientaddr;
		socklen_t len = sizeof(clientaddr);

		int s = ::accept(handle, reinterpret_cast<sockaddr*>(&clientaddr), &len);
		if (s == -1)
			return -1;

		client.handle = s;
		return 0;
	}

	int Socket::SetSendTimeout(unsigned int usec_timeout)
	{
		timeval sendrecvtimeout;
		sendrecvtimeout.tv_sec = 0;
		sendrecvtimeout.tv_usec = usec_timeout;

		if (setsockopt(handle, SOL_SOCKET, SO_SNDTIMEO, (void *)&sendrecvtimeout,sizeof(struct timeval)) != 0)
			return -1;

		return 0;
	}

	int Socket::SetRecvTimeout(unsigned int usec_timeout)
	{
		timeval sendrecvtimeout;
		sendrecvtimeout.tv_sec = 0;
		sendrecvtimeout.tv_usec = usec_timeout;

		if (setsockopt(handle, SOL_SOCKET, SO_RCVTIMEO, (void *)&sendrecvtimeout,sizeof(struct timeval)) != 0)
			return -1;

		return 0;
	}

	int Socket::SetSendBufSize(int size)
	{
		if (setsockopt(handle, SOL_SOCKET, SO_SNDBUF, (void *)&size,sizeof(size)) != 0)
			return -1;

		return 0;
	}

	int Socket::SetRecvBufSize(int size)
	{
		if (setsockopt(handle, SOL_SOCKET, SO_RCVBUF, (void *)&size,sizeof(size)) != 0)
			return -1;

		return 0;
	}

	int Socket::SetDelay(bool delay)
	{
		int flags = 0;
		if (!delay)
			flags = 1;

		if (setsockopt(handle, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags)) != 0)
			return -1;

		return 0;
	}

	int Socket::GetBlocking(bool& blocking)
	{
		int oldflags = fcntl(handle, F_GETFL, 0);
		if (oldflags == -1)
		{
			return -1;
		}
		blocking = ((oldflags & O_NONBLOCK) == 0);
		return 0;
	}

	int Socket::SetBlocking(bool blocking)
	{
		int flags = fcntl(handle, F_GETFL, 0);
		if (flags == -1)
		{
			return -1;
		}

		if (!blocking)
			flags |= O_NONBLOCK;
		else
			flags &= ~O_NONBLOCK;

		if (fcntl(handle, F_SETFL, flags) != 0)
		{
			return -1;
		}

		return 0;
	}

	void Socket::Close()
	{
		::close(handle);
	}

	int Socket::Send(char* buf, size_t buflen)
	{
		int bytes_left; 
		int written_bytes; 
		char *ptr = buf; 

		bytes_left = buflen; 
		while(bytes_left > 0) 
		{ 
			written_bytes = send(handle,ptr,bytes_left,0); 
			if(written_bytes <= 0) /* 出错了*/ 
			{ 
				if(errno == EINTR) /* 中断错误 我们继续写*/ 
					written_bytes = 0; 
				else /* 其他错误 没有办法,只好撤退了*/ 
					return written_bytes; 
			} 
			bytes_left -= written_bytes;
			if(bytes_left == 0)
				break;
			ptr += written_bytes; /* 从剩下的地方继续写 */ 
		} 
		return buflen;
	}

	int Socket::Recv(char* buf, size_t buflen)
	{
		int bytes_left; 
		int bytes_read;
		char *ptr = buf;

		bytes_left = buflen;
		while(bytes_left > 0) 
		{ 
			bytes_read = recv(handle,ptr,bytes_left,0); 
			if(bytes_read <= 0) 
			{ 
				if(errno == EINTR) 
					bytes_read = 0; 
				else
					return bytes_read;
			} 
			bytes_left -= bytes_read;

			if(bytes_left == 0)
				break;

			ptr += bytes_read;
		}
		return buflen;
	}

	int Socket::CanRecv(int sec,int usec)
	{
		timeval timeout;
		timeout.tv_sec = sec;
		timeout.tv_usec = usec;

		fd_set set;
		FD_ZERO(&set);
		FD_SET(handle, &set);

		int ret = select(handle + 1, &set, NULL, NULL, &timeout);
		return ret;
	}
	u_int32_t Socket::GetMyIp()
	{
		u_int32_t ip;
		int s;
		struct ifconf conf;
		struct ifreq *ifr;
		char buff[BUFSIZ];
		int num;
		int i;

		s = socket(PF_INET, SOCK_DGRAM, 0);
		conf.ifc_len = BUFSIZ;
		conf.ifc_buf = buff;

		ioctl(s, SIOCGIFCONF, &conf);
		num = conf.ifc_len / sizeof(struct ifreq);
		ifr = conf.ifc_req;

		for(i = 0;i < num;i++)
		{
			struct sockaddr_in *sin = (struct sockaddr_in *)(&ifr->ifr_addr);

			ioctl(s, SIOCGIFFLAGS, ifr);
			if(((ifr->ifr_flags & IFF_LOOPBACK) == 0) && (ifr->ifr_flags & IFF_UP))
			{
				/*printf("%s (%s)\n",
						ifr->ifr_name,
						inet_ntoa(sin->sin_addr));*/
				//strcpy(ip,inet_ntoa(sin->sin_addr));
				ip = (u_int32_t)(sin->sin_addr.s_addr);
				break;
			}
			ifr++;
		}
		close(s);
		return ip;
	}
} // namespace yt


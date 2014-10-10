#include "yt/network/clientsocket.h"
#include "yt/network/datadecoder.h"
#include "yt/log/log.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/un.h>
#include "yt/network/alarm.h"

#include <unistd.h>
namespace yt
{
#define MAX_THREAD_BUFFER_SIZE 10485760
	extern yt::TSS g_tss;
	void clear_buffer(void *p)
	{
		if (p)
			delete [](char*)p;
	}
	yt::TSS g_tss_buffer(clear_buffer);

	void ClientSocketBase::OnFDRead()
	{
		char *buf = (char*)g_tss_buffer.get();
		if (!buf)
		{
			buf = new char[MAX_THREAD_BUFFER_SIZE];
			g_tss_buffer.set(buf);
			AC_DEBUG("thread id: %d, tss g_tss_buffer created", pthread_self());
		}
		//char buf[65535];
		//ssize_t len = ::recv(m_fd,buf,sizeof(buf),0);
		ssize_t len = ::recv(m_fd,buf,MAX_THREAD_BUFFER_SIZE,0);
		
		//AC_DEBUG("recv buf = %s",buf);
		if(len == 0)	//正常关闭
		{
			//AC_INFO("client close normally");
			if(GetReactor()->IsETModel())
			{
				((EPETReactor *)GetReactor())->CancelFDEvent(m_fd, EPOLLIN | EPOLLOUT);
			}
			Close();
			return;
		}
		else if(len < 0)	//出错
		{
			if(GetReactor()->IsETModel())
			{
				((EPETReactor *)GetReactor())->CancelFDEvent(m_fd, EPOLLIN);
			}

			if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)//EAGAIN和EWOULDBLOCK--接受缓冲区没有数据 EINTR--中断
			{
				return;
			}
			else
			{
				if(GetReactor()->IsETModel())
				{
					((EPETReactor *)GetReactor())->CancelFDEvent(m_fd, EPOLLOUT | EPOLLIN);
				}
				perror("recv error in OnFDRead");
				//AC_INFO("client close after error");
				Close();
				return;
			}
		}
		if(m_pDecoder==NULL)
		{
			AC_ERROR("m_pDecoder is NULL !!!!!");
		}
		m_pDecoder->Process(this,buf,len);
	}
	void ClientSocketBase::OnFDWrite()
	{
		char *buf;
		size_t buflen;

		if (m_sendbuf.GetBuf(&buf, buflen) < 0)
			return;

		ssize_t len = ::send(m_fd,buf,buflen,0);
		if(len < 0)
		{
			if(GetReactor()->IsETModel())
			{
				((EPETReactor *)GetReactor())->CancelFDEvent(m_fd, EPOLLOUT);
			}
			if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)//EAGAIN和EWOULDBLOCK--发送缓冲区满 EINTR--中断
			{
				return;
			}
			else
			{
				AC_ERROR("send error,errdesc = %s",strerror(errno));
				if(GetReactor()->IsETModel())
				{
					((EPETReactor *)GetReactor())->CancelFDEvent(m_fd, EPOLLIN | EPOLLOUT);
				}
				Close();
				return;
			}
		}

		m_sendbuf.move(len);
		if (m_sendbuf.empty())
			UnRegisterWrite();

		///////////////////
		//RegisterRead((struct timeval*)NULL);
		///////////////////
		
	}
	/*int ClientSocketBase::Send(const char *buf,size_t buflen)
	{
		int len = ::send(m_fd,buf,buflen,0);
		if(len < 0)
		{
			AC_ERROR("send error,errdesc = %s",strerror(errno));
			if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)//EAGAIN和EWOULDBLOCK--发送缓冲区满 EINTR--中断
			{
				return 0;
			}
			else
			{
				return -1;
			}
		}
		return len;
	}*/
	void ClientSocketBase::OnFDReadTimeOut()
	{
		AC_ERROR("cann't recv data from client after timeout");
		Close();
	}
	void ClientSocketBase::OnFDWriteTimeOut()
	{
		AC_ERROR("cann't send data from client after timeout");
                Close();
	}
	int ClientSocketBase::SendBuf(const char* buf,size_t buflen)
	{
		if(m_sendbuf.Append(buf,buflen,SEND_CIRCLE_CACHE) != 0)
		{
			char desc[64];
			sprintf(desc,"append send buf error,ip = %s",GetPeerIp().c_str());
			Alarm2(AT_SENDBUFFULL,desc);
			AC_ERROR("append send buf error");
			return -1;
		}
		if(RegisterWrite(WRITETIMEOUT) != 0)
		{
			AC_ERROR("register write error");
			return -1;
		}
		/////////////////
		//UnRegisterRead();
		/////////////////
		return 0;
	}
	string ClientSocketBase::GetPeerIp(int fd)
	{
		sockaddr_in addr;
		socklen_t len = sizeof(sockaddr_in);
		getpeername(fd,(struct sockaddr*)&addr,&len);
		char ip[32];
		sprintf(ip,"%s",inet_ntoa(addr.sin_addr));
		return ip;
	}
	string ClientSocketBase::GetPeerIp()
	{
		/*sockaddr_in addr;
		socklen_t len = sizeof(sockaddr_in);
		getpeername(m_fd,(struct sockaddr*)&addr,&len);
		char ip[32];
		sprintf(ip,"%s",inet_ntoa(addr.sin_addr));
		return ip;*/
		return GetPeerIp(m_fd);
	}
	void ClientSocketBase::Close()
	{
		//if(IsClosed())
		//	return;

		m_recvbuf.InitPos();
		m_sendbuf.InitPos();

		FDEventHandler::Close();
                int *pflag = (int*)g_tss.get();
                if(!pflag)
                {
                        pflag = new int();
                        g_tss.set(pflag);
                }
		*pflag = 1;
		//m_IsClosed = true;
	}
	void ClientSocket::Close()
	{
		//if(IsClosed())
		//	return;
		m_pClientMap->Del(GetClientID());
		ClientSocketBase::Close();
		delete this;
		//DestroyMe();
	}
}


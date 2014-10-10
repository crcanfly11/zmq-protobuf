#include "yt/network/serversocket.h"
#include "yt/log/log.h"
#include "yt/util/timeuse.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/un.h>
#include "yt/network/alarm.h"
#include "yt/util/jsonobjguard.h"
#include <json.h>
#include "yt/util/scope_guard.h"

namespace yt
{	
	ServerSocketBase::ServerSocketBase(Reactor *pReactor,DataDecoderBase *decoder,size_t maxrecvbufsize,size_t maxsendbufsize,int packtimeout,int conntimeout,int readtimeout) : ClientSocketBase(pReactor,decoder,maxrecvbufsize,maxsendbufsize),TMEventHandler(pReactor),RDMAP(packtimeout),m_IsConnect(false),m_issupportloadbalance(false),m_conntimeout(conntimeout),m_readtimeout(readtimeout)
	{
		m_timelist.clear();
	}
	void ServerSocketBase::AddTime(long t)
	{
		m_timelist.Acquire();
		LockGuard g(&m_timelist);

		if(m_timelist.size() > SAVETIME)
			m_timelist.erase(m_timelist.begin());

		m_timelist.push_back(t);
	}
	ReserveData* ServerSocketBase::GetRD(int id)
	{
		ReserveData **rd2 = RDMAP::Get(id);
		if(!rd2)
			return NULL;
		
		return *rd2;
	}
	void ServerSocketBase::SendGSSIPack()
	{
		int seq = GetSeq();
		char outbuf[64];
		BinaryWriteStream2 writestream(outbuf,sizeof(outbuf));
		writestream.Write(GETSERVERSYSINFO_C2S2C);
		writestream.Write(seq);
		writestream.Flush();

		if(SendStream(writestream) == 0)
			new(nothrow) ReserveData(this,seq,0,0,0,RDTYPE_GETSERVERSYSINFO);
	}
	long ServerSocketBase::GetSpeed(int *slowtime)
	{
		m_timelist.Acquire();
		LockGuard g(&m_timelist);

		if(slowtime)
			(*slowtime) = 0;
		if(m_timelist.size() == 0)
			return 0;

		long sum = 0;
		vector<long>::iterator it;
		for(it = m_timelist.begin();it != m_timelist.end();it++)
		{
			if(*it == TIMEOUTFLAG)
			{
				if(slowtime)
					(*slowtime)++;
			}

			sum += *it;
		}
		return (long)((double)sum / (double)m_timelist.size());
	}
	void ServerSocketBase::GetStatus(json_object * obj)
	{
		char desc[64];
		GetDesc(desc);
		if(IsConnect())
		{
			int slowtime;
			long speed;
			speed = GetSpeed(&slowtime);
			
			json_object_object_add(obj, "server", json_object_new_string(desc));
			json_object_object_add(obj, "conn", json_object_new_string("ok"));
			json_object_object_add(obj, "speed", json_object_new_int(speed));
			json_object_object_add(obj, "slow", json_object_new_int(slowtime));
		}
		else
		{
			json_object_object_add(obj, "server", json_object_new_string(desc));
			json_object_object_add(obj, "conn", json_object_new_string("error"));
		}
	}
	/*void StateTcpServerSocket::GetStatus(json_object *obj)
	{
		TcpServerSocket::GetStatus(obj);
		if(IsConnect())
		{
			json_object_object_add(obj, "sync", json_object_new_int(m_state));
		}
	}*/
	void ServerSocketBase::Close()
	{
		//Clear();
		m_timelist.Acquire();
		m_timelist.clear();
		m_timelist.Release();

		m_IsConnect = false;
		Clear();
		ClientSocketBase::Close();
		//del by xingwenfeng 2012 12 10
		//RegisterTimer(m_conntimeout);
		Connect(0);
	}
	void ServerSocketBase::RealClose()
	{
		m_IsConnect = false;
		ClientSocketBase::Close();
		TMEventHandler::Close();
	}
	void ServerSocketBase::OnConnect()
	{
		m_IsConnect = true;
		//m_IsClosed = false;
		//RegisterTimer(m_packtimeout);
		//add by xingwenfeng 2012 12 10
		UnRegisterTimer();
		RegisterTimer(PACKTIMEOUT);
		if(m_readtimeout == 0)
		{
			if(RegisterRead((struct timeval*)NULL) != 0)
			{
				AC_ERROR("RegisterRead error");
				Close();
			}
		}
		else
		{
			if(RegisterRead(m_readtimeout) != 0)
			{
				AC_ERROR("RegisterRead error");
				Close();
			}
		}
	}
	void ServerSocketBase::OnTimeOut()
	{
		//AC_INFO("time out");
		if(!IsConnect())
		{
			if(CanWrite(0))
			{
				char desc[64];
				GetDesc(desc);
				AC_INFO("connect %s success",desc);
				OnConnect();
				//m_IsConnecting = false;
				//return;//异步连接成功返回
			}
			else
			{
				//重连
				char desc[64];
				char alarmdesc[128];
				GetDesc(desc);
				sprintf(alarmdesc,"server %s cann't connect",desc);
				Alarm2(AT_SERVERCONNERROR,alarmdesc);

				AC_INFO("connect %s error after %d sec",desc,m_conntimeout);
				close(m_fd);
				Connect(0);
			}
		}
		else
		{
			RDMAP::OnTimeOut();
			if(m_issupportloadbalance)
				SendGSSIPack();//发送获得服务器负载的包
		}
	}
	bool ServerSocketBase::CanWrite(long usec)
	{
		/*fd_set set;
		FD_ZERO(&set);
		FD_SET(fd, &set);
	
		timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = usec;

		int ret = select(fd + 1, NULL, &set, NULL, &timeout);
		if (ret > 0)
		{
			if(FD_ISSET(fd,&set))
			{
				int error = -1;
				int len = sizeof(error);
				if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len) < 0)
					return false;
				if (error != 0)
					return false;
				return true;
			}
		}
		return false;*/
		int epfd = epoll_create(2);
		AC_ON_BLOCK_EXIT(close, epfd);

		int ret;
		int op = EPOLL_CTL_ADD;
		__uint32_t evtype = EPOLLOUT;

		struct epoll_event ev;
		ev.data.fd = m_fd;
		ev.events = evtype;

		if((ret = epoll_ctl(epfd,op,m_fd,&ev)) < 0)
		{
			AC_INFO("epoll_ctl error ret[%d] usec[%d]",ret,usec);
			return false;
		}

		int i,maxi,sockfd,nfds;
		struct epoll_event events[4];
		maxi = 0;

		nfds = epoll_wait(epfd,events,4,usec);

		if(nfds < 0)
		{
			AC_INFO("epoll_wait error nfds[%d] usec[%d]",ret,usec);
			return false;
		}

		if(nfds == 0)
			AC_INFO("epoll_wait nfds[%d] usec[%d]",ret,usec);

		for(i = 0;i < nfds;++i)
		{
			if(events[i].events & EPOLLOUT)
			{
				if( (sockfd = events[i].data.fd) < 0)
					return false;
				int error = -1;
				int len = sizeof(error);
				getsockopt(m_fd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len);
				return error == 0;
			}
		}
		return false;
	}
	int ServerSocketBase::SendBuf(const char* buf,size_t buflen)
	{
		if(!IsConnect())
			return -1;

		return ClientSocketBase::SendBuf(buf,buflen);
	}
	void ServerSocketBase::OnDel(ReserveData*& pRD)
	{
		if(IsConnect())
		{
			char desc[256];
			GetDesc(desc);
			//sprintf(alarmdesc,"pack time out,svr = %s,info = %s",desc,pRD->GetInfo().c_str());
			string stralarm("pack time out,svr = ");
			stralarm.append(desc);
			stralarm.append(",info = ");
			stralarm.append(pRD->GetInfo());

			AC_ERROR("%s",stralarm.c_str());
			Alarm3(AT_PACKTIMEOUT,stralarm.c_str());

			pRD->type == RDTYPE_GETSERVERSYSINFO ? SetGSSISpeed(TIMEOUTFLAG) : AddTime(TIMEOUTFLAG);
		}
		delete pRD;
	}
	TcpServerSocket::TcpServerSocket(Reactor *pReactor,DataDecoderBase *decoder,const char* ip,int port,size_t maxrecvbufsize,size_t maxsendbufsize,int packtimeout,int conntimeout,int readtimeout) : ServerSocketBase(pReactor,decoder,maxrecvbufsize,maxsendbufsize,packtimeout,conntimeout,readtimeout),m_port(port)
	{
		strncpy(m_ip,ip,sizeof(m_ip));
	}
	int TcpServerSocket::Connect(int waittime)
	{
		//m_IsConnecting = false;
		if(IsConnect())
			return 0;
		UnRegisterTimer();
		RegisterTimer(m_conntimeout);

		AC_INFO("connect %s:%d ......",m_ip,m_port);
		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(m_port);

		addr.sin_addr.s_addr = inet_addr(m_ip);
		if (addr.sin_addr.s_addr == (in_addr_t)-1)
		{
			AC_ERROR("create socket error");
			return -1;
		}

		m_fd = socket(AF_INET,SOCK_STREAM,0);
		if(m_fd == -1)
		{
			AC_ERROR("create socket error");
			return -1;
		}
		if(SetNonBlocking() != 0)
		{
			AC_ERROR("SetNonBlocking error");
			return -1;
		}
		int ret = ::connect(m_fd,(struct sockaddr*)&addr,sizeof(addr));
		//AC_INFO("connect ret = %d",ret);
		if(ret != 0 && errno != EINPROGRESS)
		{
			close(m_fd);
			AC_ERROR("connect error");
			return -1;
		}
		if(CanWrite(waittime))
		{
			AC_INFO("connect %s:%d success",m_ip,m_port);
			OnConnect();
		}
		else
		{
			AC_INFO("connect %s:%d error after %d usec",m_ip,m_port,waittime);
		}

		return 0;
	}	
	void TcpServerSocket::GetDesc(char *desc)
	{
		sprintf(desc,"ip:port = %s:%d",m_ip,m_port);
	}
	
	UnixDomainSocket::UnixDomainSocket(Reactor *pReactor,DataDecoderBase *decoder,const char* path,size_t maxrecvbufsize,size_t maxsendbufsize,int packtimeout,int conntimeout,int readtimeout) : ServerSocketBase(pReactor,decoder,maxrecvbufsize,maxsendbufsize,packtimeout,conntimeout,readtimeout)
	{
		strncpy(m_path,path,sizeof(m_path));
	}
	void UnixDomainSocket::GetDesc(char *desc)
	{
		sprintf(desc,"path = %s",m_path);
	}
	int UnixDomainSocket::Connect(int waittime)
	{
		if(IsConnect())
			return 0;
		//add by xingwenfeng 2012-12-10
		UnRegisterTimer();
		RegisterTimer(m_conntimeout);

		AC_INFO("connect %s ......",m_path);
		sockaddr_un addr;
		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		sprintf(addr.sun_path,"CLIENTPATH_%d_%d",getpid(),(int)pthread_self());

		int len = offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path);
;
		unlink(addr.sun_path);

		m_fd = socket(AF_UNIX,SOCK_STREAM,0);
		if(m_fd == -1)
		{
			AC_ERROR("create socket error");
			return -1;
		}
		if(SetNonBlocking() != 0)
		{
			AC_ERROR("SetNonBlocking error");
			return -1;
		}

		if(bind(m_fd,(struct sockaddr*)&addr,len) < 0)
		{
			AC_ERROR("bind client socket error");
			return -1;
		}

		memset(&addr,0,sizeof(addr));
		addr.sun_family = AF_UNIX;
		strcpy(addr.sun_path,m_path);
		len = offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path);

		int ret = ::connect(m_fd,(struct sockaddr*)&addr,len);
		//AC_INFO("connect ret = %d",ret);
		if(ret != 0 && errno != EINPROGRESS)
		{
			close(m_fd);
			AC_ERROR("connect error");
			return -1;
		}
		if(CanWrite(waittime))
		{
			AC_INFO("connect %s success",m_path);
			OnConnect();
		}
		else
		{
			AC_INFO("connect %s error after %ld usec",m_path,CONNWAITTIME);
		}

		return 0;
	}
}

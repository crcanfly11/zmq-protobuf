#include "yt/network/netapp.h"
#include "yt/network/socket.h"
#include "yt/util/timeuse.h"
#include "yt/util/bufguard.h"
#include <errno.h>
#include <string.h>
#include <memory>

namespace yt
{
#define IS_RECV_ERROR \
	if(recvlen == 0){ \
		AC_DEBUG("client close"); \
		close(m_fd); \
		break; \
	} \
	else if(recvlen < 0){ \
		AC_DEBUG("client close recv error,err = %s",strerror(errno)); \
		close(m_fd); \
		break; \
	}

	ClientThread::ClientThread(int fd,size_t maxoneinpacklen,int timeout,DataProcessor *processor,TSList<ClientThread*> *threadlist,TSVector<long> *timelist) : 
		m_fd(fd),
		m_maxoneinpacklen(maxoneinpacklen),
		m_timeout(timeout),
		m_processor(processor),
		m_threadlist(threadlist),
		m_timelist(timelist),
		m_flag(true){}
	ClientThread::~ClientThread()
	{
		m_threadlist->Acquire();
		m_threadlist->remove(this);
		m_threadlist->Release();
	}
	void ClientThread::Run()
	{
		auto_ptr<ClientThread> ag(this);
		Socket socket;
		socket.SetHandle(m_fd);

		if(socket.SetRecvTimeout(3000000) != 0)//recv接收超时3秒
		{
			AC_ERROR("set recv time out error");
			return;
		}
		if(socket.SetSendTimeout(3000000) != 0) //send发送超时3秒
		{
			AC_ERROR("set send time out error");
			return;
		}

		time_t conntime = time(NULL);
		while(m_flag)
		{
			int ret = socket.CanRecv(0,200000);
			if (ret < 0)
			{
				AC_ERROR("select error");
				break;
			}
			else if(ret == 0)
			{
				if(time(NULL) - conntime > m_timeout)
				{
					AC_ERROR("cann't recv data from client after timeout");
					break;
				}
				//AC_DEBUG("select timeout");
				continue;
			}
			conntime = time(NULL);

			int packlen;
			ssize_t recvlen = socket.Recv((char*)&packlen,HEADER_LEN_4);	//先收包头4个字节

			IS_RECV_ERROR;

			int packlen2 = ntohl(packlen);
			if((size_t)packlen2 > m_maxoneinpacklen)
			{
				AC_ERROR("pack too long,len = %d",packlen2);
				break;
			}

			char *inbuf = (char*)malloc(packlen2);
			if(!inbuf)
			{
				AC_ERROR("allocate inbuf error");
				break;
			}
			BufGuard bg(inbuf,true);

			memcpy(inbuf,&packlen,HEADER_LEN_4);

			recvlen = socket.Recv(inbuf + HEADER_LEN_4,packlen2 - HEADER_LEN_4);	//再收包体

			IS_RECV_ERROR;

			timeval begin;gettimeofday(&begin,NULL);
			string outbuf;
			if(m_processor->Process(inbuf,packlen2,outbuf) != 0)	//数据处理,输出在outbuf里
			{
				AC_DEBUG("processor ret != 0");
				break;
			}
			timeval end;gettimeofday(&end,NULL);
			
			//统计速度
			m_timelist->Acquire();
			if(m_timelist->size() > 1000)
			{
				m_timelist->erase(m_timelist->begin());
			}
			m_timelist->push_back(time_use(&begin,&end));
			m_timelist->Release();
			//--------------


			if(socket.Send(&outbuf[0],outbuf.length()) < 0)	//发回客户端
			{
				AC_DEBUG("client close send error,err = %s",strerror(errno));
				break;
			}
		}
		close(m_fd);
	}
	long NetApp2::GetSpeed()
	{
		m_timelist.Acquire();
		LockGuard g(&m_timelist);

		if(m_timelist.size() == 0)
			return 0;
	
		long sum = 0;
		for(size_t i = 0;i < m_timelist.size();i++)
		{
			sum += m_timelist[i];
		}
		return (long)((double)sum / (double)m_timelist.size());
	}
	int NetApp2::GetClientCount()
	{
		m_threadlist.Acquire();
		LockGuard g(&m_threadlist);
		return m_threadlist.size();
	}
	bool NetApp2::Start()
	{
		int fd = socket(AF_INET,SOCK_STREAM,0);
		if(fd == -1)
		{
			AC_ERROR("create socket error");
			return false;
		}

		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(m_port);
		addr.sin_addr.s_addr = inet_addr(m_host.c_str());

		int i = 1;
		if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void*)&i, sizeof(int)) == -1)
		{
			AC_ERROR("setsocketopt error");
			return false;
		}
		if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
		{
			AC_ERROR("bind error");
			return false;
		}

		if(::listen(fd,500) != 0)
		{
			AC_ERROR("listen error");
			return false;
		}

		while(m_flag)
		{
			Socket socket;
			socket.SetHandle(fd);

			int ret = socket.CanRecv(0,200000);
			if (ret < 0)
			{
				/*if(errno == EINTR)
				{
					//AC_INFO("errno == EINTR");
					continue;
				}*/

				AC_ERROR("select error,%s",strerror(errno));
				continue;
				//break;
			}
			else if(ret == 0)
			{
				//AC_DEBUG("select timeout");
				continue;
			}

			sockaddr_in clientaddr;
			socklen_t len = sizeof(clientaddr);

			//AC_DEBUG("a client connect");
			int fd2 = ::accept(fd, reinterpret_cast<sockaddr*>(&clientaddr), &len);
			if (fd2 == -1)
			{
				AC_ERROR("accept error,errdesc = %s",strerror(errno));
				continue;
			}
			AC_DEBUG("a client connect,ip = %s",ClientSocketBase::GetPeerIp(fd2).c_str());

			m_threadlist.Acquire();
			LockGuard g(&m_threadlist);

			if(m_threadlist.size() >= (size_t)m_maxclient)
			{
				AC_ERROR("too many client");
				Alarm3(AT_CLIENTOVERFLOW,AD_CLIENTOVERFLOW);
				close(fd2);
				continue;
			}
			
			if(!OnAccept(fd2))
				continue;

			ClientThread *thread = new ClientThread(fd2,m_maxoneinpacklen,m_timeout,m_processor,&m_threadlist,&m_timelist);	//启动一个线程并发一个连接

			m_threadlist.push_back(thread);
			thread->Start(PTHREAD_CREATE_DETACHED);
			//thread->Start();
		}
		AC_ERROR("stop listen thread");
		close(fd);
		
		m_threadlist.Acquire();
		for(list<ClientThread*>::iterator it = m_threadlist.begin();it != m_threadlist.end();it++)
		{
			ClientThread *p = *it;
			p->Stop();
			//sleep(1);
			AC_INFO("stop client thread");
		}
		m_threadlist.Release();

		for(int i = 0;i < 60;i++)
		{
			m_threadlist.Acquire();
			if(m_threadlist.size() == 0)
			{
				m_threadlist.Release();
				break;
			}
			m_threadlist.Release();
		
			sleep(1);
		}

		return true;
	}
	void NetApp2::SendAll(char *buf,size_t buflen)
	{
		m_threadlist.Acquire();
		LockGuard g(&m_threadlist);

		TSList<ClientThread*>::iterator it;
		for(it = m_threadlist.begin();it != m_threadlist.end();it++)
		{
			int fd = (*it)->m_fd;
			Socket s;
			s.SetHandle(fd);
			s.Send(buf,buflen);
		}	
	}			
}


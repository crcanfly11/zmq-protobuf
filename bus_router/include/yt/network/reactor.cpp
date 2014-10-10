#include <sys/epoll.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <list>
#include "yt/network/reactor.h"
#include "yt/network/eventhandler.h"
#include "yt/log/log.h"
#include "yt/util/timeuse.h"
#include "yt/util/minheapqueue.h"

namespace yt
{
	void EPReactor::RegisterIdle(IdleEventHandler *pHandler)
	{
		m_Set.AddIdleEventHandler(pHandler);
	}
	void EPReactor::UnRegisterIdle(IdleEventHandler *pHandler)
	{
		m_Set.DelIdleEventHandler(pHandler);
	}
	void EPReactor::RegisterTimer(TMEventHandler *pHandler)
	{
		m_Set.AddTMEventHandler(pHandler);
	}
	bool EPReactor::IsETModel()
	{
		return false;
	}
	int EPReactor::RegisterReadEvent(FDEventHandler *pHandler,struct timeval *timeout)
	{
		int ret;
		int op;
		int flag = 0;
		__uint32_t evtype = EPOLLIN;
		FDEventHandler *p = m_Set.GetFDEventHandler(pHandler->GetFD(),&flag);
		if(p == NULL)
		{ 
			op = EPOLL_CTL_ADD;
		}
		else
		{
			op = EPOLL_CTL_MOD;
			
			if(flag & EPOLLOUT)
				evtype = evtype | EPOLLOUT;
		}
		
		struct epoll_event ev;
		ev.data.fd = pHandler->GetFD();
		ev.events = evtype;
		
		if((ret = epoll_ctl(m_epfd,op,pHandler->GetFD(),&ev)) < 0)
		{
			AC_ERROR("epoll_ctl < 0 %s, errno %d, fd:%d, op:%d",strerror(errno),errno,pHandler->GetFD(),op);
			return -1;
		}

		m_Set.AddFDEventHandler(pHandler,EPOLLIN, timeout);
		return 0;
	}
	int EPReactor::RegisterWriteEvent(FDEventHandler *pHandler,struct timeval *timeout)
	{
		int ret;
		int op;
		int flag = 0;
		__uint32_t evtype = EPOLLOUT;
		FDEventHandler *p = m_Set.GetFDEventHandler(pHandler->GetFD(),&flag);
		if(p == NULL)
		{ 
			op = EPOLL_CTL_ADD;
		}
		else
		{
			op = EPOLL_CTL_MOD;
			if(flag & EPOLLIN)
				evtype = evtype | EPOLLIN;
		}
		
		struct epoll_event ev;
		ev.data.fd = pHandler->GetFD();
		ev.events = evtype;
		
		if((ret = epoll_ctl(m_epfd,op,pHandler->GetFD(),&ev)) < 0)
		{
			AC_ERROR("epoll_ctl < 0 %s, errno %d, fd:%d, op:%d",strerror(errno),errno,pHandler->GetFD(),op);
			return -1;
		}

		m_Set.AddFDEventHandler(pHandler,EPOLLOUT,timeout);
		return 0;
	}
	void EPReactor::Stop()
	{
		//close();
		m_running = false;
		m_stoptime = time(NULL) + 10;		// 10秒后仍无法停止，直接退出
	}
	int EPReactor::Init()
	{
		m_epfd = epoll_create(256);
		if(m_epfd < 0)
			return -1;
		return 0;
	}
	void EPReactor::Run()
	{
		int i,maxi,sockfd,nfds;
		struct epoll_event events[200];//以前是40
		maxi = 0;

		while(1)
		{
			nfds = epoll_wait(m_epfd,events,200,m_running?100:1);
			//if(nfds == 0)
			//	AC_DEBUG("ndfs = 0");
			//AC_DEBUG("ndfs = %d",nfds);
			
			if (!m_running)
			{
				if (nfds <= 0)
					break;
					
				time_t cur = time(NULL);
				if (cur >= m_stoptime)
				{
					AC_INFO("epoll run over 10 sec when stopping program.");
					break;
				}
			}

			if(nfds < 0)
			{
				if(errno!=EINTR)
					AC_ERROR("ndfs < 0 %s",strerror(errno));
			}
				for(i = 0;i < nfds;++i)
			{
				if(events[i].events & EPOLLIN)	//读事件
				{
					//AC_DEBUG("events[i].events & EPOLLIN");
					if ( (sockfd = events[i].data.fd) < 0) 
						continue;
					FDEventHandler *pHandler = m_Set.GetFDEventHandler(sockfd,NULL);
					if(!pHandler)
					{
						AC_ERROR("pHandler == NULL, fd %d", sockfd);
						continue;
					}
					pHandler->OnFDRead();
				}
				if(events[i].events & EPOLLOUT)	//写事件
				{
					//AC_DEBUG("events[i].events & EPOLLOUT");  
					if( (sockfd = events[i].data.fd) < 0)
						continue;
					FDEventHandler *pHandler = m_Set.GetFDEventHandler(sockfd,NULL);
					if(!pHandler)
					{
						AC_ERROR("pHandler == NULL, fd %d", sockfd);
						continue;
					}
					pHandler->OnFDWrite();
				}
				if(events[i].events & EPOLLERR)	//异常事件
				{
					//AC_DEBUG("EPOLLERR");  
					if( (sockfd = events[i].data.fd) < 0)
						continue;
					FDEventHandler *pHandler = m_Set.GetFDEventHandler(sockfd,NULL);
					if(pHandler)
					{
						pHandler->Close();		//2010-2-12修改
						//UnRegisterEvent(pHandler);	//
					}
				}
			}

			m_Set.Scan();
			m_Set.Idle();
		}
		AC_INFO("epoll stop");
		close(m_epfd);
	}

	void EPReactor::UnRegisterTimer(TMEventHandler *pHandler)
	{
		m_Set.DelTMEventHandler(pHandler);
	}
	int EPReactor::UnRegisterReadEvent(FDEventHandler *pHandler)
	{
		int op;
		int flag = 0;
		__uint32_t evtype = EPOLLIN;
		FDEventHandler *p = m_Set.GetFDEventHandler(pHandler->GetFD(),&flag);
		if(p == NULL)
		{ 
			op = EPOLL_CTL_DEL;
		}
		else
		{
			if(flag & EPOLLOUT)
			{
				op = EPOLL_CTL_MOD;
				evtype = EPOLLOUT;
			}
			else
			{
				op = EPOLL_CTL_DEL;
			}
		}
		
		struct epoll_event ev;
		ev.data.fd = pHandler->GetFD();
		ev.events = evtype;
		
		//(isaddormod == true) ? op = EPOLL_CTL_ADD:op = EPOLL_CTL_MOD;
		if(epoll_ctl(m_epfd,op,pHandler->GetFD(),&ev) < 0)
		{
			AC_ERROR("epoll_ctl < 0 %s, errno %d, fd:%d, op:%d",strerror(errno),errno,pHandler->GetFD(),op);
			return -1;
		}

		m_Set.DelFDEventHandler(pHandler->GetFD(),EPOLLIN);
		return 0;
	}
	int EPReactor::UnRegisterWriteEvent(FDEventHandler *pHandler)
	{
		int op;
		int flag = 0;
		__uint32_t evtype = EPOLLOUT;
		FDEventHandler *p = m_Set.GetFDEventHandler(pHandler->GetFD(),&flag);
		if(p == NULL)
		{ 
			op = EPOLL_CTL_DEL;
		}
		else
		{
			if(flag & EPOLLIN)
			{
				op = EPOLL_CTL_MOD;
				evtype = EPOLLIN;
			}
			else
			{
				op = EPOLL_CTL_DEL;
			}
		}
		
		struct epoll_event ev;
		ev.data.fd = pHandler->GetFD();
		ev.events = evtype;
		
		//(isaddormod == true) ? op = EPOLL_CTL_ADD:op = EPOLL_CTL_MOD;
		if(epoll_ctl(m_epfd,op,pHandler->GetFD(),&ev) < 0)
		{
			AC_ERROR("epoll_ctl < 0 %s, errno %d, fd:%d, op:%d",strerror(errno),errno,pHandler->GetFD(),op);
			return -1;
		}

		m_Set.DelFDEventHandler(pHandler->GetFD(),EPOLLOUT);
		return 0;
	}
	int EPReactor::UnRegisterEvent(FDEventHandler *pHandler)
	{
		struct epoll_event ev;
		ev.data.fd = pHandler->GetFD();
		epoll_ctl(m_epfd,EPOLL_CTL_DEL,pHandler->GetFD(),&ev);

		m_Set.DelFDEventHandler(pHandler->GetFD(),EPOLLIN | EPOLLOUT);
		return 0;
	}


	////////////////////////////////////////
	void EPETReactor::RegisterIdle(IdleEventHandler *pHandler)
	{
		m_Set.AddIdleEventHandler(pHandler);
	}
	void EPETReactor::UnRegisterIdle(IdleEventHandler *pHandler)
	{
		m_Set.DelIdleEventHandler(pHandler);
	}
	void EPETReactor::RegisterTimer(TMEventHandler *pHandler)
	{
		m_Set.AddTMEventHandler(pHandler);
	}
	bool EPETReactor::IsETModel()
	{
		return true;
	}
	int EPETReactor::RegisterReadEvent(FDEventHandler *pHandler,struct timeval *timeout)
	{
		int ret;
		int op;
		int flag = 0;
		if(pHandler->SetNonBlocking(true) != 0)
		{
			AC_ERROR("set nonblocking error");
			pHandler->Close();
			return -1;
		}
		__uint32_t evtype = EPOLLIN | EPOLLET;
		FDEventHandler *p = m_Set.GetFDEventHandler(pHandler->GetFD(),&flag);
		if(p == NULL)
		{ 
			op = EPOLL_CTL_ADD;
		}
		else
		{
			op = EPOLL_CTL_MOD;
			
			if(flag & EPOLLOUT)
				evtype = evtype | EPOLLOUT;
		}
		
		struct epoll_event ev;
		ev.data.fd = pHandler->GetFD();
		ev.events = evtype;
		
		if((ret = epoll_ctl(m_epfd,op,pHandler->GetFD(),&ev)) < 0)
		{
			AC_ERROR("epoll_ctl < 0 %s, errno %d, fd:%d, op:%d",strerror(errno),errno,pHandler->GetFD(),op);
			return -1;
		}

		m_Set.AddFDEventHandler(pHandler,EPOLLIN, timeout);
		return 0;
	}
	int EPETReactor::RegisterWriteEvent(FDEventHandler *pHandler,struct timeval *timeout)
	{
		int ret;
		int op;
		int flag = 0;
		if(pHandler->SetNonBlocking(true) != 0)
		{
			AC_ERROR("set nonblocking error");
			pHandler->Close();
			return -1;
		}
		__uint32_t evtype = EPOLLOUT | EPOLLET;
		FDEventHandler *p = m_Set.GetFDEventHandler(pHandler->GetFD(),&flag);
		if(p == NULL)
		{ 
			op = EPOLL_CTL_ADD;
		}
		else
		{
			op = EPOLL_CTL_MOD;
			if(flag & EPOLLIN)
				evtype = evtype | EPOLLIN;
		}
		
		struct epoll_event ev;
		ev.data.fd = pHandler->GetFD();
		ev.events = evtype;
		
		if((ret = epoll_ctl(m_epfd,op,pHandler->GetFD(),&ev)) < 0)
		{
			AC_ERROR("epoll_ctl < 0 %s, errno %d, fd:%d, op:%d",strerror(errno),errno,pHandler->GetFD(),op);
			return -1;
		}

		m_Set.AddFDEventHandler(pHandler,EPOLLOUT,timeout);
		return 0;
	}
	void EPETReactor::Stop()
	{
		//close();
		m_running = false;
		m_stoptime = time(NULL) + 10;		// 10秒后仍无法停止，直接退出
	}
	int EPETReactor::Init()
	{
		m_epfd = epoll_create(256);
		if(m_epfd < 0)
			return -1;
		return 0;
	}
	void EPETReactor::Run()
	{
		int i,maxi,sockfd,nfds;
		struct epoll_event events[200];//以前是40
		maxi = 0;

		while(1)
		{
			nfds = epoll_wait(m_epfd,events,200,m_running?100:1);
			//if(nfds == 0)
			//	AC_DEBUG("ndfs = 0");
			//AC_DEBUG("ndfs = %d",nfds);
			
			if (!m_running)
			{
				if (nfds <= 0)
					break;
					
				time_t cur = time(NULL);
				if (cur >= m_stoptime)
				{
					AC_INFO("epoll run over 10 sec when stopping program.");
					break;
				}
			}

			if(nfds < 0)
			{
				if(errno!=EINTR)
					AC_ERROR("ndfs < 0 %s",strerror(errno));
			}
				for(i = 0;i < nfds;++i)
			{
				if(events[i].events & EPOLLIN)	//读事件
				{
					//AC_DEBUG("events[i].events & EPOLLIN");
					if ( (sockfd = events[i].data.fd) < 0) 
						continue;
				/*
					FDEventHandler *pHandler = m_Set.GetFDEventHandler(sockfd,NULL);
					if(!pHandler)
					{
						AC_ERROR("pHandler == NULL");
						continue;
					}
			        pHandler->OnFDRead();
				*/
					m_Set.MarkFDEvent(sockfd, EPOLLIN);
				}
				if(events[i].events & EPOLLOUT)	//写事件
				{
					//AC_DEBUG("events[i].events & EPOLLOUT");  
					if( (sockfd = events[i].data.fd) < 0)
						continue;
				/*
					FDEventHandler *pHandler = m_Set.GetFDEventHandler(sockfd,NULL);
					if(!pHandler)
					{
						AC_ERROR("pHandler == NULL");
						continue;
					}
			        pHandler->OnFDWrite();
				*/	
					m_Set.MarkFDEvent(sockfd, EPOLLOUT);
				}
				if(events[i].events & EPOLLERR)	//异常事件
				{
					//AC_ERROR("EPOLLERR");  
					if( (sockfd = events[i].data.fd) < 0)
						continue;
					FDEventHandler *pHandler = m_Set.GetFDEventHandler(sockfd,NULL);
					if(pHandler)
					{
						pHandler->Close();		//2010-2-12修改
						UnRegisterEvent(pHandler);	//
					}
				}
			}

			ProcFDEvent();
			m_Set.Scan();
			m_Set.Idle();
		}
		AC_INFO("epoll stop");
		close(m_epfd);
	}
	
	void EPETReactor::CancelFDEvent(int fd, int event)
	{
		m_Set.CancelFDEvent(fd, event);
	}

	/* 扫描事件 Map，逐个处理 */
	void EPETReactor::ProcFDEvent()
	{
		hash_map<int, int>::iterator it;
		hash_map<int, int> m_FDEvent = m_Set.GetFDEvent();
		for(it=m_FDEvent.begin(); it != m_FDEvent.end(); it++)
		{
			FDEventHandler *pHandler = m_Set.GetFDEventHandler(it->first,NULL);
			if(!pHandler)
			{
				AC_ERROR("pHandler == NULL, fd:%d", it->first);
				CancelFDEvent(it->first, EPOLLIN | EPOLLOUT);
				continue;
			}
			
			if(it->second & EPOLLIN)
			{
				pHandler->OnFDRead();
			}
			if(it->second & EPOLLOUT)
			{
				FDEventHandler *pHandler = m_Set.GetFDEventHandler(it->first,NULL);
				if(pHandler)
					pHandler->OnFDWrite();
			}
		}//end for
	}
	
	void EPETReactor::UnRegisterTimer(TMEventHandler *pHandler)
	{
		m_Set.DelTMEventHandler(pHandler);
	}
	int EPETReactor::UnRegisterReadEvent(FDEventHandler *pHandler)
	{
		int op;
		int flag = 0;
		__uint32_t evtype = EPOLLIN;
		FDEventHandler *p = m_Set.GetFDEventHandler(pHandler->GetFD(),&flag);
		if(p == NULL)
		{ 
			op = EPOLL_CTL_DEL;
		}
		else
		{
			if(flag & EPOLLOUT)
			{
				op = EPOLL_CTL_MOD;
				evtype = EPOLLOUT | EPOLLET;
			}
			else
			{
				op = EPOLL_CTL_DEL;
			}
		}
		
		struct epoll_event ev;
		ev.data.fd = pHandler->GetFD();
		ev.events = evtype;
		
		//(isaddormod == true) ? op = EPOLL_CTL_ADD:op = EPOLL_CTL_MOD;
		if(epoll_ctl(m_epfd,op,pHandler->GetFD(),&ev) < 0)
		{
			AC_ERROR("epoll_ctl < 0 %s, errno %d, fd:%d, op:%d",strerror(errno),errno,pHandler->GetFD(),op);
			return -1;
		}

		m_Set.DelFDEventHandler(pHandler->GetFD(),EPOLLIN);
	//	m_Set.CancelFDEvent(pHandler->GetFD(), EPOLLIN);
		return 0;
	}
	int EPETReactor::UnRegisterWriteEvent(FDEventHandler *pHandler)
	{
		int op;
		int flag = 0;
		__uint32_t evtype = EPOLLOUT;
		FDEventHandler *p = m_Set.GetFDEventHandler(pHandler->GetFD(),&flag);
		if(p == NULL)
		{ 
			op = EPOLL_CTL_DEL;
		}
		else
		{
			if(flag & EPOLLIN)
			{
				op = EPOLL_CTL_MOD;
				evtype = EPOLLIN | EPOLLET;
			}
			else
			{
				op = EPOLL_CTL_DEL;
			}
		}
		
		struct epoll_event ev;
		ev.data.fd = pHandler->GetFD();
		ev.events = evtype;
		
		//(isaddormod == true) ? op = EPOLL_CTL_ADD:op = EPOLL_CTL_MOD;
		if(epoll_ctl(m_epfd,op,pHandler->GetFD(),&ev) < 0)
		{
			AC_ERROR("epoll_ctl < 0 %s, errno %d, fd:%d, op:%d",strerror(errno),errno,pHandler->GetFD(),op);
			return -1;
		}

		m_Set.DelFDEventHandler(pHandler->GetFD(),EPOLLOUT);
		m_Set.CancelFDEvent(pHandler->GetFD(), EPOLLOUT);
		return 0;
	}
	int EPETReactor::UnRegisterEvent(FDEventHandler *pHandler)
	{
		struct epoll_event ev;
		ev.data.fd = pHandler->GetFD();
		epoll_ctl(m_epfd,EPOLL_CTL_DEL,pHandler->GetFD(),&ev);

		m_Set.DelFDEventHandler(pHandler->GetFD(),EPOLLIN | EPOLLOUT);
		m_Set.CancelFDEvent(pHandler->GetFD(), EPOLLIN | EPOLLOUT);
		return 0;
	}
	///////////////////////////////////////////////////////
	
	EventHandlerSet::EventHandlerSet()
	{
		m_lasttime.tv_sec = 0;
		m_lasttime.tv_usec = 0;
		m_TMEHQueue=new TMMinHeapQueue;
	}
	void EventHandlerSet::MarkFDEvent(int fd, int event)
	{
		hash_map<int, int>::iterator it = m_FDEvent.find(fd);
		if(it != m_FDEvent.end())
		{
			it->second |= event;
		}
		else
		{
			m_FDEvent.insert(make_pair(fd, event));
		}
	}
	void EventHandlerSet::CancelFDEvent(int fd, int event)
	{
		hash_map<int, int>::iterator it = m_FDEvent.find(fd);
		if(it == m_FDEvent.end())
		{
			return;
		}
		if((it->second &= (~event)) == 0)
		{
			m_FDEvent.erase(fd);
		}

	}
	/////////////////

	void EventHandlerSet::AddFDEventHandler(FDEventHandler *pHandler,int mask,struct timeval *timeout)
	{
		hash_map<int,FDEHINFO>::iterator it = m_FDEHMap.find(pHandler->GetFD());
		if(it != m_FDEHMap.end())
		{
			it->second.pHandler = pHandler;
			it->second.flag |= mask;
		}
		else
		{
			FDEHINFO info;
			info.pHandler = pHandler;
			info.flag = mask;
			m_FDEHMap.insert(make_pair(pHandler->GetFD(),info));
		}
	}	
	void EventHandlerSet::AddTMEventHandler(TMEventHandler *pHandler)
	{
		//m_TMEHList.insert(pHandler);
		m_TMEHQueue->Push(pHandler);
	}
	void EventHandlerSet::AddIdleEventHandler(IdleEventHandler *pHandler)
	{
		m_IdleEHList.insert(pHandler);
	}
	void EventHandlerSet::DelIdleEventHandler(IdleEventHandler *pHandler)
	{
		m_IdleEHList.erase(pHandler);
	}
	void EventHandlerSet::DelFDEventHandler(int fd,int mask)
	{
		hash_map<int,FDEHINFO >::iterator it = m_FDEHMap.find(fd);
		if(it == m_FDEHMap.end())
			return;
			
		if((it->second.flag &= (~mask)) == 0)
		{
			m_FDEHMap.erase(fd);
		}
	}
	void EventHandlerSet::DelTMEventHandler(TMEventHandler *pHandler)
	{
		//m_TMEHList.erase(pHandler);
		m_TMEHQueue->Del(pHandler);
	}
	EventHandlerSet::~EventHandlerSet()
	{
		if(m_TMEHQueue)
		{
			delete m_TMEHQueue;
			m_TMEHQueue=NULL;
		}

	}
	FDEventHandler* EventHandlerSet::GetFDEventHandler(int fd,int *flag )
	{	 
		hash_map<int,FDEHINFO>::iterator it;
		it = m_FDEHMap.find(fd);
		if(it == m_FDEHMap.end())
			return NULL;
		if(flag)
			*flag = it->second.flag;

		return it->second.pHandler;
	}
	void EventHandlerSet::Idle()
	{
		set<IdleEventHandler*>::iterator it;
		for(it = m_IdleEHList.begin();it != m_IdleEHList.end();it++)
		{
			(*it)->OnRun();
		}
	}
	void EventHandlerSet::Scan()
	{
		//static struct timeval lasttime = {0,0};
		struct timeval nowtime;
		gettimeofday(&nowtime,NULL);
		long timeuse = time_use(&m_lasttime,&nowtime);
		if(timeuse < 200000 && m_lasttime.tv_sec != 0)//0.2秒跑一次
		{
			return;
		}
                
		/*
		set<TMEventHandler*>::iterator it2;
		vector<TMEventHandler*> v;
		gettimeofday(&nowtime,NULL);
		for(it2 = m_TMEHList.begin();it2 != m_TMEHList.end();it2++)
		{
			long timeuse = time_use(&((*it2)->regtime),&nowtime);
			if(timeuse >= ((*it2)->to.tv_sec*1000000L + (*it2)->to.tv_usec))
			{
				(*it2)->regtime = nowtime;
				v.push_back(*it2);
			}
		}
		for(size_t i = 0;i < v.size();i++)
		{
			v[i]->OnTimeOut();
		}
		 */
		while(!m_TMEHQueue->IsEmpty())
		{
			TMEventHandler* handler=m_TMEHQueue->Front();
			if(handler)
			{
				long timediff=time_use(&(handler->endtime),&nowtime);
				if(timediff >=0)
				{
					m_TMEHQueue->Del(handler);
					handler->endtime.tv_sec=nowtime.tv_sec + handler->to.tv_sec;
					handler->endtime.tv_usec=nowtime.tv_usec + handler->to.tv_usec;
					m_TMEHQueue->Push(handler);
					handler->OnTimeOut();
				}
				else 
				{
					break;
				}

			}
			else
			     break;

		}
		gettimeofday(&m_lasttime,NULL);
	}
}

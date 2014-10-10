#include "signalhandle.h"
#include "yt/log/log.h"

namespace yt
{
	void on_signal(int signo)
	{
		int signoval = htonl(signo);
			send(SignalStruct::Instance().GetFd(signo),(char*)&signoval,sizeof(int),0);
	}

	void SignalEventHandlerBase::OnFDRead()
	{
		int signo;
			int len = recv(sockfd[1],(char*)&signo,sizeof(int),0);
			if(len<0)
			{
					return;
			}
			else
			{
					signo = ntohl(signo);
					OnSignal(signo);
			}
	}

	bool SignalEventHandlerBase::Init()
	{
		SignalStruct::Instance().Init();
		socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd);
		SetFD(sockfd[1]);	

		if(FDEventHandler::SetNonBlocking(sockfd[0],true) != 0)
		{
			AC_ERROR("set non blocing error");
			return false;
		}		
		if(FDEventHandler::SetNonBlocking(sockfd[1],true) != 0)
		{		
			AC_ERROR("set non blocing error");
			return false;
		}

		if(RegisterRead((struct timeval*)NULL) != 0)
		{		
			AC_ERROR("register socket pair error");
			return false;
		}
		return true;
	}

	void SignalEventHandler::OnSignal(int signo)
	{
		AC_DEBUG("catch signo = %d",signo);
	}
}

#pragma once
#include "yt/network/eventhandler.h"
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <map>
#include <assert.h>

namespace yt
{
	#define MAX_SIG_NUM 32
	class SignalStruct
	{
	   public:
			static SignalStruct& Instance()
			{
					static SignalStruct ss;
					return ss;
			}
			void Init()
			{
	
					for(int i=0;i < MAX_SIG_NUM;i++)
			{
				sigfd[i] = -1;
			}
			}
			void SignalSetFd(int signo,int fd)
			{
					sigfd[signo] = fd;
			}
			int GetFd(int signo)
			{
					return sigfd[signo];
			}
			map<int,sig_atomic_t> sigfd;
	};

	void on_signal(int signo);

	class SignalEventHandlerBase : public FDEventHandler
	{
		public:
			SignalEventHandlerBase(Reactor *pReactor):FDEventHandler(pReactor)
			{
				if(!Init())
					assert(0);
			}
					~SignalEventHandlerBase(){}
		
			virtual void OnFDRead();
			virtual void OnFDWrite(){};
			virtual void OnFDReadTimeOut(){};
			virtual void OnFDWriteTimeOut(){};

			virtual void OnSignal(int signo)=0;
	
			inline void RegisterSigno(int signo)
			{
				signal(signo, on_signal);
				SignalStruct::Instance().SignalSetFd(signo,sockfd[0]);
			}

		protected:
			bool Init();	
			int sockfd[2];
	};

	class SignalEventHandler: public SignalEventHandlerBase
	{       
			public:
					SignalEventHandler(Reactor *pReactor):SignalEventHandlerBase(pReactor){}                                                     
			~SignalEventHandler(){};
		
			void OnSignal(int signo);
	};  
}

#pragma once

#include "yt/network/eventhandler.h"
#include "yt/util/counter.h"
#include <arpa/inet.h>

namespace yt
{
	//tcpº‡Ã˝¿‡ª˘¿‡
	class ListenSocketBase : public FDEventHandler
	{
		const static int MAXCLIENTREADTIMEOUT = 2000;
		public:
			ListenSocketBase(const char *host,int port,Reactor *pReactor,int clientreadtimeout);
			ListenSocketBase(int fd,Reactor *pReactor,int clientreadtimeout);
			virtual ~ListenSocketBase(){}
			int Listen();
			virtual void OnFDRead();
			virtual void OnAccept(int fd) = 0;
			virtual void OnFDWrite(){}
			virtual void OnFDReadTimeOut(){}
			virtual void OnFDWriteTimeOut(){}
		protected:
			char m_host[20];
			int m_port;
			int m_clientreadtimeout;
	};
	class UnixListenSocketBase : public FDEventHandler
	{
		const static int MAXCLIENTREADTIMEOUT = 2000;
		public:
			UnixListenSocketBase(const char *path,Reactor *pReactor,int clientreadtimeout);
			UnixListenSocketBase(int fd,Reactor *pReactor,int clientreadtimeout);
			virtual ~UnixListenSocketBase(){}
			int Listen();
			virtual void OnFDRead();
			virtual void OnAccept(int fd) = 0;
			virtual void OnFDWrite(){}
			virtual void OnFDReadTimeOut(){}
			virtual void OnFDWriteTimeOut(){}
		protected:
			char m_path[256];
			int m_clientreadtimeout;
	};
	class UdpListenSocketBase : public FDEventHandler
	{
		public:
			UdpListenSocketBase(const char *host,Reactor *pReactor,int port) : FDEventHandler(pReactor),m_host(host),m_port(port){}
			int Listen();
			virtual void OnFDRead(){}
			virtual void OnFDWrite(){}
			virtual void OnFDReadTimeOut(){}
			virtual void OnFDWriteTimeOut(){}
		private:
			string m_host;
			int m_port;
	};
	class UdpListenSocketBase2 : public UdpListenSocketBase
	{
		public:
			UdpListenSocketBase2(const char* host,Reactor *pReactor,int port) : UdpListenSocketBase(host,pReactor,port){}
			virtual void OnFDRead();
			virtual int ProcessData(char *inbuf,size_t inbuflen,char *outbuf,size_t &outbuflen,sockaddr *psockddr) = 0;
	};
}

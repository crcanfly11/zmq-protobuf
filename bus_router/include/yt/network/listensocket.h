#pragma once

#include "yt/network/listensocketbase.h"
#include "yt/network/clientsocket.h"
#include "yt/network/datadecoder.h"
#include "yt/log/log.h"
#include "yt/network/alarm.h"

namespace yt
{
	class ClientSocket;
	class DataDecoderBase;
	class ClientMap;
	
	template <class CLIENTSOCKET = ClientSocket>
		class UnixListenSocket : public UnixListenSocketBase
		{
			public:
				UnixListenSocket(const char *path,Reactor *pReactor,int clientreadtimeout,DataDecoderBase *pDecoder,ClientMap *pClientMap,int maxclient,size_t maxinbufsize,size_t maxoutbufsize) : 
					UnixListenSocketBase(path,pReactor,clientreadtimeout),
					m_pDecoder(pDecoder),
					m_pClientMap(pClientMap),
					m_maxclient(maxclient),
					m_maxinbufsize(maxinbufsize),
					m_maxoutbufsize(maxoutbufsize){}

				UnixListenSocket(int fd,Reactor *pReactor,int clientreadtimeout,DataDecoderBase *pDecoder,ClientMap *pClientMap,int maxclient,size_t maxinbufsize,size_t maxoutbufsize) : 
					UnixListenSocketBase(fd,pReactor,clientreadtimeout),
					m_pDecoder(pDecoder),
					m_pClientMap(pClientMap),
					m_maxclient(maxclient),
					m_maxinbufsize(maxinbufsize),
					m_maxoutbufsize(maxoutbufsize){}

				virtual void OnAccept(int fd)
				{
					if((int)m_pClientMap->Size() > m_maxclient)
					{
						AC_ERROR("too many client");
						Alarm3(AT_CLIENTOVERFLOW,AD_CLIENTOVERFLOW);
						::close(fd);
						return;
					}

					CLIENTSOCKET *pClient = new(std::nothrow) CLIENTSOCKET(GetReactor(),m_pDecoder,m_pClientMap,fd,m_maxinbufsize,m_maxoutbufsize);

					if(!pClient)
					{
						AC_ERROR("Cann't Allocate client in pool");
						Alarm3(AT_MEMERROR,"cann't allocate client");
						::close(fd);
						return;
					}

					int seq = m_Counter.Get();
					while(  ( m_pClientMap->Get(seq) )  != NULL   )
					{
						AC_INFO("seq overlap");
						seq = m_Counter.Get();
					}
					pClient->SetClientID(seq);
					m_pClientMap->Put(seq,pClient);

					if(pClient->RegisterRead(m_clientreadtimeout) != 0)
					{
						AC_ERROR("register read error");
						pClient->Close();
						return;
					}
				}
			private:
				Counter m_Counter;
				DataDecoderBase *m_pDecoder;
				ClientMap *m_pClientMap;
				int m_maxclient;

				const size_t m_maxinbufsize;
				const size_t m_maxoutbufsize;
		};
	//tcp监听类,accept后创建一个CLIENTSOCKET类型的客户端socket类,CLIENTSOCKET可以继承自ClientSocket
	typedef void (*OnAccept2)(void *);
	template <class CLIENTSOCKET = ClientSocket>
		class ListenSocket : public ListenSocketBase/*,public ObjPool<CLIENTSOCKET>*/
		{
			public:
				ListenSocket(const char *host,int port,Reactor *pReactor,int clientreadtimeout,DataDecoderBase *pDecoder,ClientMap *pClientMap,int maxclient,size_t maxinbufsize,size_t maxoutbufsize) : 
					ListenSocketBase(host,port,pReactor,clientreadtimeout),
					m_pDecoder(pDecoder),
					m_pClientMap(pClientMap),
					m_maxclient(maxclient),
					m_maxinbufsize(maxinbufsize),
					m_maxoutbufsize(maxoutbufsize),
					m_fun(NULL){}

				ListenSocket(int fd,Reactor *pReactor,int clientreadtimeout,DataDecoderBase *pDecoder,ClientMap *pClientMap,int maxclient,size_t maxinbufsize,size_t maxoutbufsize) : 
					ListenSocketBase(fd,pReactor,clientreadtimeout),
					m_pDecoder(pDecoder),
					m_pClientMap(pClientMap),
					m_maxclient(maxclient),
					m_maxinbufsize(maxinbufsize),
					m_maxoutbufsize(maxoutbufsize),
					m_fun(NULL){}

				virtual void OnAccept(int fd)
				{
					if((int)m_pClientMap->Size() > m_maxclient)
					{
						AC_ERROR("too many client");
						Alarm3(AT_CLIENTOVERFLOW,AD_CLIENTOVERFLOW);
						::close(fd);
						return;
					}

					//CLIENTSOCKET *pClient = new(std::nothrow) CLIENTSOCKET(GetReactor(),m_pDecoder,m_maxinbufsize,m_maxoutbufsize);
					//CLIENTSOCKET *pClient = new(std::nothrow) CLIENTSOCKET();
					//CLIENTSOCKET *pClient = ObjPool<CLIENTSOCKET>::Create();
					CLIENTSOCKET *pClient = new(std::nothrow) CLIENTSOCKET(GetReactor(),m_pDecoder,m_pClientMap,fd,m_maxinbufsize,m_maxoutbufsize);	//创建客户端对象

					if(!pClient)
					{
						AC_ERROR("Cann't Allocate client in pool");
						Alarm3(AT_MEMERROR,"cann't allocate client");
						::close(fd);
						return;
					}
					//pClient->SetFD(fd);
					//pClient->SetDcb(this);
					/*pClient->SetReactor(m_pReactor);
					pClient->SetDataDecoder(m_pDecoder);
					pClient->GetRecvbuf()->Init(m_maxinbufsize);
					pClient->GetSendbuf()->Init(m_maxoutbufsize);*/

					int seq = m_Counter.Get();
					while(  ( m_pClientMap->Get(seq) )  != NULL   )
					{
						AC_INFO("seq overlap");
						seq = m_Counter.Get();
					}
					pClient->SetClientID(seq);	//设置客户端的id
					//pClient->SetClientMap(m_pClientMap);

					/*char ip[32];
					pClient->GetPeerIp(ip);
					AC_DEBUG("accept ip = %s",ip);*/

					m_pClientMap->Put(seq,pClient);	//放入clientmap
					//pClient->Open();

					if(pClient->RegisterRead(m_clientreadtimeout) != 0)	//注册读事件
					{
						AC_ERROR("register read error");
						pClient->Close();
						return;
					}
					if(m_fun)
						m_fun(pClient);
				}
				inline void SetOnAccept2(OnAccept2 fun){
					m_fun = fun;
				}			
			private:
				Counter m_Counter;
				DataDecoderBase *m_pDecoder;
				ClientMap *m_pClientMap;
				int m_maxclient;

				const size_t m_maxinbufsize;
				const size_t m_maxoutbufsize;
				OnAccept2 m_fun;
		};
}


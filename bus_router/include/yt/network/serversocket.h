#pragma once

#include "yt/network/clientsocket.h"
#include "yt/network/tmseqmap.h"
#include "yt/util/counter.h"
#include "yt/util/tsvector.h"
#include "reservedata.h"
#include <json.h>

namespace yt
{
	//对后面的服务器类
	class ServerSocketBase : public ClientSocketBase , public TMEventHandler,public RDMAP
	{
		protected:
			static const long CONNWAITTIME = 5000;
			static const int CONNTIMEOUT = 2;
			static const int PACKTIMEOUT = 2;
			static const int TIMEOUTFLAG = 100000000;
			static const size_t SAVETIME = 50;
		public:
			ServerSocketBase(Reactor *pReactor,DataDecoderBase *decoder,size_t maxrecvbufsize = MAXRECVBUFSIZE,size_t maxsendbufsize = MAXSENDBUFSIZE,int packtimeout = PACKTIMEOUT,int conntimeout = CONNTIMEOUT,int readtimeout = 0);

			inline bool IsConnect(){return m_IsConnect;}

			virtual int Connect(int waittime = CONNWAITTIME) = 0;	//建立连接
			virtual void GetDesc(char *desc) = 0;

			virtual void OnConnect();
			virtual void OnTimeOut();

			void AddTime(long t);
			long GetSpeed(int *slowtime = NULL);
			//virtual void GetStatus(char *str);
			virtual void GetStatus(json_object *arrobj);
			virtual int SendBuf(const char* buf,size_t buflen);
			virtual void Close();
			void RealClose();

			bool CanWrite(long usec);
			virtual void OnDel(ReserveData*& pRD);
			ReserveData* GetRD(int id);
			void SendGSSIPack();

			inline int GetSeq(){return m_counter.Get();}
			inline long GetGSSISpeed() const{return m_gssispeed;}
			inline void SetGSSISpeed(long speed){m_gssispeed = speed;}
			inline int GetServerLoad() const{return m_serverload;}
			inline void SetServerLoad(int load){m_serverload = load;}
			inline void SupportLoadBalance(){m_issupportloadbalance = true;}
			inline void NotSupportLoadBalance(){m_issupportloadbalance = false;}
			inline bool IsSupportLoadBalance() const{return m_issupportloadbalance;}
		private:
			bool m_IsConnect;
			Counter m_counter;
			bool m_issupportloadbalance;
			long m_gssispeed;	//最近一个GETSERVERSYSINFO包的返回时间
			int m_serverload;	//最近一次返回的服务器的负载
		protected:
			TSVector<long> m_timelist;
			int m_conntimeout;
			int m_readtimeout;
	};
	class TcpServerSocket : public ServerSocketBase
	{
		public:
			TcpServerSocket(Reactor *pReactor,DataDecoderBase *decoder,const char* ip,int port,size_t maxrecvbufsize = MAXRECVBUFSIZE,size_t maxsendbufsize = MAXSENDBUFSIZE,int packtimeout = PACKTIMEOUT,int conntimeout = CONNTIMEOUT,int readtimeout = 0);
			inline char* GetIp(){return m_ip;}
			inline int GetPort(){return m_port;}
			virtual int Connect(int waittime = CONNWAITTIME);
			virtual void GetDesc(char *desc);
		private:
			char m_ip[20];
			int m_port;
	};
	class StateTcpServerSocket : public TcpServerSocket
	{
		public:
			static const short QUERYSTATE_C2S2C = -2;
			StateTcpServerSocket(Reactor *pReactor,DataDecoderBase *decoder,const char* ip,int port,size_t maxrecvbufsize = MAXRECVBUFSIZE,size_t maxsendbufsize = MAXSENDBUFSIZE,int packtimeout = PACKTIMEOUT,int conntimeout = CONNTIMEOUT,int readtimeout = 0) : TcpServerSocket(pReactor,decoder,ip,port,maxrecvbufsize,maxsendbufsize,packtimeout,conntimeout,readtimeout),m_state(0){}
			inline bool IsOk() const{
				return m_state == 0;
			}
			inline void SetState(int state){
				m_state = state;
			}
			//virtual void GetStatus(json_object *arrobj);
		private:
			int m_state;	//0--ok 非0错误
	};
	class UnixDomainSocket : public ServerSocketBase
	{
		public:
			UnixDomainSocket(Reactor *pReactor,DataDecoderBase *decoder,const char* path,size_t maxrecvbufsize = MAXRECVBUFSIZE,size_t maxsendbufsize = MAXSENDBUFSIZE,int packtimeout = PACKTIMEOUT,int conntimeout = CONNTIMEOUT,int readtimeout = 0);
			inline char* GetPath(){return m_path;}
			virtual int Connect(int waittime = CONNWAITTIME);
			virtual void GetDesc(char *desc);
		private:
			char m_clientpath[256];
			char m_path[256];
	};
#define PROCESSSERVERSYSINFO(CMD,STREAM,SVR,T)\
	if(CMD == GETSERVERSYSINFO_C2S2C)\
	{\
		int load;\
		if(!STREAM.Read(load))\
		{\
			AC_ERROR("read load error");\
			return 0;\
		}\
		char desc[64];\
		SVR->GetDesc(desc);\
		RUNLOG("GETSERVERSYSINFO_C2S2C,svr = %s,load = %d,gssispeed = %d",desc,load,T);\
		SVR->SetServerLoad(load);\
		SVR->SetGSSISpeed(T);\
		return 0;\
	}
}

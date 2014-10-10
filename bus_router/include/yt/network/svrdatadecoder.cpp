#include "yt/network/svrdatadecoder.h"
#include "yt/network/clientsocket.h"
#include "yt/network/serversocket.h"
#include "yt/util/timeuse.h"

namespace yt
{
	int SvrDataDecoderBase::OnPackage(ClientSocketBase *client,const char *buf,size_t buflen)
	{
		ServerSocketBase *svr = (ServerSocketBase*)client;

		BinaryReadStream2 stream(buf,buflen);
		/*if(!stream.CheckSum())
		{
			AC_ERROR("check sum error");
			return 0;
		}*/

		short cmd;
		if(!stream.Read(cmd))
		{
			AC_ERROR("read cmd error");
			svr->Close();
			return -1;
		}

		int seq;
		if(!stream.Read(seq))
		{
			AC_ERROR("read seq error");
			svr->Close();
			return -1;
		}

		if(seq == 0)		//seq = 0不需要处理，可能是写cache之类
			return 0;

		ReserveData *rd = svr->GetRD(seq);	//获得保留数据
		if(!rd)
		{
		  	AC_ERROR("cann't find rd");
		  	return 0;
		}
		rd->count--;	//引用计数-1
		ReserveDataGuard g(svr,seq);
		
		timeval now;
		gettimeofday(&now,NULL);
		long t = time_use(&(rd->t),&now);
		
		PROCESSSERVERSYSINFO(cmd,stream,svr,t)

		svr->AddTime(t);	//获得统计的响应速度

		ClientSocket* frontclient = m_clientmap->Get(rd->clientseq);	//获得前端客户端对象
		if(!frontclient)
		{
			AC_ERROR("cann't find front client");
			return 0;
		}

		return Process2(svr,buf,buflen,rd,frontclient);	//业务处理
	}
	int StateSvrDataDecoderBase::OnPackage(ClientSocketBase *client,const char *buf,size_t buflen)	//流程基本同上，加入了状态的控制
	{
		StateTcpServerSocket *svr = (StateTcpServerSocket*)client;

		BinaryReadStream2 stream(buf,buflen);
		/*if(!stream.CheckSum())
		{
			AC_ERROR("check sum error");
			return 0;
		}*/

		short cmd;
		if(!stream.Read(cmd))
		{
			AC_ERROR("read cmd error");
			svr->Close();
			return -1;
		}

		int seq;
		if(!stream.Read(seq))
		{
			AC_ERROR("read seq error");
			svr->Close();
			return -1;
		}

		if(seq == 0)		//seq = 0不需要处理，可能是写cache之类
		{
			if(cmd == StateTcpServerSocket::QUERYSTATE_C2S2C)	//设置状态的协议命令字
			{
				int state;
				if(!stream.Read(state))
				{
					AC_ERROR("read state error");
					svr->Close();
					return -1;
				}
				char desc[64];
				svr->GetDesc(desc);
				AC_INFO("%s state = %d",desc,state);
				svr->SetState(state);
			}
			return 0;
		}

		ReserveData *rd = svr->GetRD(seq);
		if(!rd)
		{
		  	AC_ERROR("cann't find rd");
		  	return 0;
		}
		rd->count--;
		ReserveDataGuard g(svr,seq);
		
		timeval now;
		gettimeofday(&now,NULL);
		long t = time_use(&(rd->t),&now);
		
		PROCESSSERVERSYSINFO(cmd,stream,svr,t)

		svr->AddTime(t);

		ClientSocket* frontclient = m_clientmap->Get(rd->clientseq);
		if(!frontclient)
		{
			AC_ERROR("cann't find front client");
			return 0;
		}

		return Process2(svr,buf,buflen,rd,frontclient);
	}
}

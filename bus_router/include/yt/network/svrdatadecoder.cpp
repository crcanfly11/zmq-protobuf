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

		if(seq == 0)		//seq = 0����Ҫ����������дcache֮��
			return 0;

		ReserveData *rd = svr->GetRD(seq);	//��ñ�������
		if(!rd)
		{
		  	AC_ERROR("cann't find rd");
		  	return 0;
		}
		rd->count--;	//���ü���-1
		ReserveDataGuard g(svr,seq);
		
		timeval now;
		gettimeofday(&now,NULL);
		long t = time_use(&(rd->t),&now);
		
		PROCESSSERVERSYSINFO(cmd,stream,svr,t)

		svr->AddTime(t);	//���ͳ�Ƶ���Ӧ�ٶ�

		ClientSocket* frontclient = m_clientmap->Get(rd->clientseq);	//���ǰ�˿ͻ��˶���
		if(!frontclient)
		{
			AC_ERROR("cann't find front client");
			return 0;
		}

		return Process2(svr,buf,buflen,rd,frontclient);	//ҵ����
	}
	int StateSvrDataDecoderBase::OnPackage(ClientSocketBase *client,const char *buf,size_t buflen)	//���̻���ͬ�ϣ�������״̬�Ŀ���
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

		if(seq == 0)		//seq = 0����Ҫ����������дcache֮��
		{
			if(cmd == StateTcpServerSocket::QUERYSTATE_C2S2C)	//����״̬��Э��������
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

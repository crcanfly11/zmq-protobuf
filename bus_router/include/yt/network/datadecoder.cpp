#include "yt/network/datadecoder.h"
#include "yt/network/clientsocket.h"
#include "yt/log/log.h"
#include "yt/network/alarm.h"
#include <netinet/in.h>
#include <sys/time.h>
#include <string.h>
#include <netproxythreadcluster.h>

namespace yt
{
	yt::TSS g_tss(Clear_Flag);

	void Clear_Flag(void *p)
	{
		if(p)
		{
			delete (int*)p;
			p = NULL;
		}
	}

	void DataDecoder_helloworld::Process(ClientSocketBase *pClient,const char *buf,size_t buflen)
	{
		if(OnPackage(pClient,buf,buflen) != 0)
		{
			return;
		}
	}
	int DataDecoder_helloworld::OnPackage(ClientSocketBase *pClient,const char *buf,size_t buflen)
	{
		if(pClient->SendBuf(buf,buflen) < 0)
		{
			pClient->Close();
			return -1;
		}
		return 0;
	}
	size_t DataDecoder::GetBuflen(const char *buf)
	{
		if(m_pttype == PROTOCOLTYPE_BINARY)	//�����ư�ͷ
		{
			if(m_hdlen == HEADER_LEN_2)	//2�ֽڵİ�ͷ����
			{
				uint16_t len;
				memcpy(&len,buf,sizeof(len));
				len = ntohs(len);
				return (size_t)len;
			}
			else if(m_hdlen == HEADER_LEN_4)//4�ֽڵİ�ͷ����
			{
				uint32_t len;
				memcpy(&len,buf,sizeof(len));
				len = ntohl(len);
				return (size_t)len;
			}
		}
		else if(m_pttype == PROTOCOLTYPE_TEXT)	//�ı���ͷ
		{
			char lenbuf[10] = {0};
			memcpy(lenbuf,buf,m_hdlen);
			long int len = strtol(lenbuf, NULL, 16);
			return (size_t)len;
		}
		return 0;
	}
	void DataDecoder::Process(ClientSocketBase *pClient,const char *buf2,size_t buflen)
	{
		int *pflag = (int*)g_tss.get();
		if(!pflag)
		{
			pflag = new int();
			g_tss.set(pflag);
		}
		*pflag = 0;
		DataBlock *recvdb = pClient->GetRecvbuf();
		const char *ptr,*ptr2;
		const char *end;
		if(recvdb->GetPos() == 0)	//���ջ������ǿ�
		{
			ptr = buf2;
			end = ptr + buflen;
		}
		else
		{
			if(recvdb->Append(buf2,buflen) < 0)
			{
				char info[64];
				sprintf(info,"append send buf error,ip = %s",pClient->GetPeerIp().c_str());
				Alarm2(AT_RECVBUFFULL,info);
				AC_ERROR("recvbuf append error");
				pClient->Close();
				return;
			}
			ptr = recvdb->GetBuf();
			end = ptr + recvdb->GetPos();
		}
		while(1)
		{
			if(*pflag == 1)
			{
				*pflag = 0;
				return;
			}
			if( size_t(end - ptr) <= m_hdlen)
				break;

			size_t buflen = GetBuflen(ptr);
			if(buflen <= 0 || buflen > m_maxonepacklen)
			{
				AC_ERROR("get buflen error,buflen = %d",buflen);
				pClient->Close();
				return;
			}

			if( ptr + buflen > end )//�����
				break;

			ptr2 = ptr;
			ptr += buflen;

			if(OnPackage(pClient,ptr2,buflen) != 0)//�������ﷵ��-1,���ٴ����Ժ�İ���
				return;

			//if(pClient->IsClosed())//�������ر������ӵ������˷���-1
			//	return;

			//ptr += buflen;
		}
		if( end - ptr == 0)//û�а��,������ջ�����
		{
			recvdb->InitPos();
		}
		else//���һ���ǰ��
		{
			if(recvdb->Copy(0,ptr,end - ptr) != 0)
			{
				AC_ERROR("recvdb->Copy != 0");
				pClient->Close();
				return;
			}
		}
	}
	void DataDecoder_DFix::Process(ClientSocketBase *pClient,const char *buf,size_t buflen)
	{
		int *pflag = (int*)g_tss.get();
		if(!pflag)
		{
			pflag = new int();
			g_tss.set(pflag);
		}
		*pflag = 0;
		DataBlock *recvdb = pClient->GetRecvbuf();
		const char *ptr,*ptr2;
		const char *end;
		if(recvdb->GetPos() == 0)
		{
			ptr = buf;
			end = ptr + buflen;
		}
		else
		{
			if(recvdb->Append(buf,buflen) < 0)
			{
				char info[64];
				sprintf(info,"append send buf error,ip = %s",pClient->GetPeerIp().c_str());
				Alarm2(AT_RECVBUFFULL,info);
				AC_ERROR("recvbuf append error");
				pClient->Close();
				return;
			}
			ptr = recvdb->GetBuf();
			end = ptr + recvdb->GetPos();
		}

		while(1)
		{
			if(*pflag == 1)
			{
				*pflag = 0;
				return;
			}	
			if(end - ptr <= 4)//����һ����,0--����,����"L="�Ĳ�ȫ
			{
				break;
			}
			if(strncmp(ptr,"L=",2) != 0)
			{
				AC_ERROR("ptr != L=");
				pClient->Close();
				return;
			}
			int i;
			for(i = 0;i < 10 || i < end - ptr - 2;i++)
			{
				if(*(ptr + 2 + i) == SOH)
				{
					break;
				}
			}
			if(i == 10 || i == 0)//L=�ֶβ���
			{
				AC_ERROR("cann't find soh");
				pClient->Close();
				return;
			}
			if(i == end - ptr - 2)//L=����
			{
				break;
			}

			/*const char *cur = ptr + 2;
			  while(cur < cur + 10 && cur < buf + pos)
			  {
			  if(*cur == SOH)
			  {
			  break;
			  }
			  cur++;
			  }*/

			char lenstr[10];
			memcpy(lenstr,ptr + 2,i);
			lenstr[i] = 0;

			size_t hdrlen,packlen;
			hdrlen = 2 + i + 1;
			packlen = atoi(lenstr) + hdrlen;

			if(packlen > m_maxonepacklen)
			{
				AC_ERROR("get buflen error");
				pClient->Close();
				return;
			}
			//size_t buflen = packlen;

			if( ptr + packlen > end )//�����
				break;

			ptr2 = ptr;
			ptr += packlen;	
			if(OnPackage(pClient,ptr2,packlen) != 0)//�������ﷵ��-1,���ٴ����Ժ�İ���
				return;

			//if(pClient->IsClosed())//�������ر������ӵ������˷���-1
			//	return;

			//ptr += packlen;
		}
		if(end - ptr == 0)//û�а��,������ջ�����
		{
			recvdb->InitPos();
		}
		else//���һ���ǰ��
		{
			if(recvdb->Copy(0,ptr,end - ptr) != 0)
			{
				AC_ERROR("recvdb->Copy != 0");
				pClient->Close();
				return;
			}
		}
	}
	int DataDecoder_Q::OnPackage(ClientSocketBase *pClient,const char *buf,size_t buflen)
	{
		ClientSocket *clientsocket = (ClientSocket*)pClient;
		pair<int,string> *processpair = new pair<int,string>;
		processpair->first = clientsocket->GetClientID();
		processpair->second.append(buf,buflen);
		if(m_processqueue->Push(processpair) < 0)
		{
			delete processpair;
			AC_ERROR("push process queue error");
		}
		return 0;
	}
	int DataDecoder_Q2::OnPackage(ClientSocketBase *pClient,const char *buf,size_t buflen)
	{
		ClientSocket *clientsocket = (ClientSocket*)pClient;
		pair<int,string> *processpair = new pair<int,string>;
		processpair->first = clientsocket->GetClientID();
		processpair->second.append(buf,buflen);
		// ��processpair�У��ڷ�װһ��(id)
		if(m_processqueue->Push(make_pair(m_threadid, processpair)) < 0)
		{
			delete processpair;
			AC_ERROR("push process queue error");
		}
		return 0;
	}
	int DataDecoder_Cluster::OnPackage(ClientSocketBase *pClient,const char *buf,size_t buflen)
	{
		if(!m_threadcluster)
		{
			AC_ERROR("m_threadcluster is null,please setthreadcluster before use");
			return -1;
		}
		ClientSocket *clientsocket = (ClientSocket*)pClient;
		int clientid = clientsocket->GetClientID();

		if(m_threadcluster->Handle(clientid,buf,buflen) != 0)
		{
			AC_ERROR("error occured in Handle request");
		}
		return 0;
	}
}



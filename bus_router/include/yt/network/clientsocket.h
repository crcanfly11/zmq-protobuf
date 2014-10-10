#pragma once

#include "yt/network/eventhandler.h"
#include "yt/network/clientmap.h"
#include "yt/util/protocolstream.h"
#include "yt/util/freesizememallocator.h"
#include "yt/util/seqmap.h"
#include "yt/util/tss.h"
#include "yt/util/data_block.h"

namespace yt
{
	class DataDecoderBase;
	class ClientMap;
	//所有网络socket类的基类
	class ClientSocketBase : public FDEventHandler
	{
		protected:
			static const int WRITETIMEOUT = 5;
			static const size_t MAXRECVBUFSIZE = 65535;
			static const size_t MAXSENDBUFSIZE = 65535;

			//bool m_IsClosed;
		public:
			ClientSocketBase(Reactor *pReactor,DataDecoderBase *decoder,size_t maxrecvbufsize = MAXRECVBUFSIZE,size_t maxsendbufsize = MAXSENDBUFSIZE) : FDEventHandler(pReactor),/*m_IsClosed(false),*/m_pDecoder(decoder),m_recvbuf(maxrecvbufsize),m_sendbuf(maxsendbufsize)
			{
			}
			/*inline void SetDataDecoder(DataDecoderBase *decoder){
				m_pDecoder = decoder;
			}*/
			/*virtual void Open(){
				m_IsClosed = false;
			}*/
			virtual ~ClientSocketBase(){}
			virtual void OnFDRead();
			virtual void OnFDWrite();
			virtual void OnFDReadTimeOut();
			virtual void OnFDWriteTimeOut();
			inline DataBlock* GetRecvbuf(){	
				return &m_recvbuf;
			}
			inline DataBlock2* GetSendbuf(){
				return &m_sendbuf;
			}
			inline int SendStream(const WriteStreamImpl &stream)
			{
				return SendBuf(stream.GetData(),stream.GetSize());
			}
			inline int SendStream(const WriteStreamImpl *stream)
			{
				return SendBuf(stream->GetData(),stream->GetSize());
			}
			virtual int SendBuf(const char *buf,size_t buflen);
			string GetPeerIp();
			static string GetPeerIp(int fd);
			/*inline bool IsClosed() const{
				return m_IsClosed;
			}*/
			virtual void Close();
		private:
			DataDecoderBase *m_pDecoder;
			DataBlock m_recvbuf;
			DataBlock2 m_sendbuf;
	};
	//连上来的客户端的基类
	class ClientSocket : /*public Destroyable,*/public ClientSocketBase//,public FSMPoolableThread
	{
		public:
			ClientSocket(Reactor *pReactor,DataDecoderBase *decoder,ClientMap *pClientMap,int fd,size_t maxrecvbufsize = MAXRECVBUFSIZE,size_t maxsendbufsize = MAXSENDBUFSIZE) : ClientSocketBase(pReactor,decoder,maxrecvbufsize,maxsendbufsize),m_pClientMap(pClientMap)
			{
				if(decoder == NULL)
				{
					printf(" datadecoder null -------------\n");
				}
				SetFD(fd);
			}
			//inline void SetClientMap(ClientMap *pClientMap){m_pClientMap = pClientMap;}
			inline void SetClientID(int id){m_id = id;}
                        virtual int GetClientID(){return m_id;}
			virtual void Close();
		private:
			ClientMap *m_pClientMap;
			int m_id;
	};
	static const short GETSERVERSYSINFO_C2S2C = -1;

#define RETURNGETSYSINFO(CMD,SEQ,PCLIENTSOCKET)\
	if(CMD == GETSERVERSYSINFO_C2S2C)\
	{\
		struct sysinfo s_info;\
		int error;\
		error = sysinfo(&s_info);\
		if(error == 0)\
		{\
			RUNLOG("GETSERVERSYSINFO_C2S2C,load = %d",(int)s_info.loads[0]);\
			char outbuf[64];\
			BinaryWriteStream2 writestream(outbuf,sizeof(outbuf));\
			writestream.Write(CMD);\
			writestream.Write(SEQ);\
			writestream.Write((int)s_info.loads[0]);\
			writestream.Flush();\
			if(PCLIENTSOCKET->SendStream(writestream) != 0)\
			return -1;\
		}\
		return 0;\
	}
#define GONEGETSYSINFO(CMD) \
	if(CMD == GETSERVERSYSINFO_C2S2C)\
		return 0;
}



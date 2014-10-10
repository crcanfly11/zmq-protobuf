#pragma once

#include "yt/network/datadecoder.h"
#include "yt/network/servercluster.h"
#include "yt/util/protocolstream.h"
#include "yt/util/bufguard.h"

using namespace yt;

namespace yt
{
	class ClientSocketBase;
	class ServerSocketBase;
	class ReserveData;
	class ClientMap;
	//服务器之间的业务处理基类
	class SvrDataDecoderBase : public DataDecoder
	{
		public:
			SvrDataDecoderBase(size_t maxoneinpacklen,ClientMap *clientmap) : DataDecoder(PROTOCOLTYPE_BINARY,HEADER_LEN_4,maxoneinpacklen),m_clientmap(clientmap){}//传入clientmap作为客户端列表
			virtual int OnPackage(ClientSocketBase *client,const char *buf,size_t buflen);
			virtual int Process2(ServerSocketBase *server,const char *buf,size_t buflen,ReserveData *rd,ClientSocket *frontclient) = 0;
		protected:
			ClientMap *m_clientmap;
	};
	class StateSvrDataDecoderBase : public DataDecoder
	{
		public:
			StateSvrDataDecoderBase(size_t maxoneinpacklen,ClientMap *clientmap) : DataDecoder(PROTOCOLTYPE_BINARY,HEADER_LEN_4,maxoneinpacklen),m_clientmap(clientmap){}
			virtual int OnPackage(ClientSocketBase *client,const char *buf,size_t buflen);
			virtual int Process2(ServerSocketBase *server,const char *buf,size_t buflen,ReserveData *rd,ClientSocket *frontclient) = 0;
		protected:
			ClientMap *m_clientmap;
	};
	class SvrDataDecoder : public SvrDataDecoderBase
	{
		public:
			SvrDataDecoder(size_t maxoneinpacklen,ClientMap *clientmap,ServerClusterMap *scmap) : SvrDataDecoderBase(maxoneinpacklen,clientmap),m_scmap(scmap){}	//传入scmap作为服务器集群对象
		protected:
			ServerClusterMap *m_scmap;
	};
	class StateSvrDataDecoder : public StateSvrDataDecoderBase
	{
		public:
			StateSvrDataDecoder(size_t maxoneinpacklen,ClientMap *clientmap,ServerClusterMap *scmap) : StateSvrDataDecoderBase(maxoneinpacklen,clientmap),m_scmap(scmap){}
		protected:
			ServerClusterMap *m_scmap;
	};
}


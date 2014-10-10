#pragma once

#include "yt/network/eventhandler.h"
#include "socketpairdatadecoderbase.h"


namespace yt
{
	class SocketPairBase : public FDEventHandler
	{
		public:
			SocketPairBase(Reactor *pReactor, SocketPairDataDecoderBase *decoder) : FDEventHandler(pReactor), m_pDecoder(decoder)
			{

			}
			virtual ~SocketPairBase(){}
			virtual void OnFDRead();
			virtual void OnFDWrite(){};
			virtual void OnFDReadTimeOut(){};
			virtual void OnFDWriteTimeOut(){};
		private:
			SocketPairDataDecoderBase *m_pDecoder;
	};
}

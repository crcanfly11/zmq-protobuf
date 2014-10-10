#pragma once

namespace yt
{
	class SocketPairBase;

	class SocketPairDataDecoderBase
	{
		public:
			SocketPairDataDecoderBase(){}
			virtual ~SocketPairDataDecoderBase(){}
			virtual int Process(SocketPairBase *pSocketPair, const char *buf, size_t buflen) = 0;
			virtual int OnPackage(SocketPairBase *pSocketPair, const char *buf, size_t buflen) = 0;
	};
}

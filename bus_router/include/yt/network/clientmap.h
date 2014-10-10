#pragma once

#include "yt/util/seqmap.h"

namespace yt
{
	//客户端id对应的map
	class ClientSocket;
	class ClientMap : public SeqMap<ClientSocket*>
	{
		public:
			void Close();
			ClientSocket* Get(int seq);
			void SendAll(const char *buf,size_t buflen);
	};
}

#ifndef TMBACKCLIENTSOCKET_H_
#define TMBACKCLIENTSOCKET_H_

#include "./network/clientsocket.h"
#include "ac/util/protocolstream.h"

namespace ac
{
	class TMBackClientSocketBase : public TcpBackClientSocket
	{
		public:
			TMBackClientSocketBase(Reactor *pReactor,const char* ip,int port,int testtime = 10) :
				TcpBackClientSocket(pReactor,ip,port),
				m_testtime(testtime),
				m_testspeedstream(m_testspeedbuf,sizeof(m_testspeedbuf))
			{}
			virtual void OnSlow();
			virtual void OnTimeOut();
		private:
			int m_testtime;
		protected:
			BinaryWriteStream m_testspeedstream;
			char m_testspeedbuf[64];
	};
}
#endif

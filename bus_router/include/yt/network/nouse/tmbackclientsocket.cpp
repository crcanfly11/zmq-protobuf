#include "tmbackclientsocket.h"
#include "ac/log/log.h"
#include "ac/util/timeuse.h"

namespace ac
{
	void TMBackClientSocketBase::OnSlow()
	{
		RegisterTimer(GetTimeout());
	}

	void TMBackClientSocketBase::OnTimeOut()
	{
		if(!IsConnect())
		{
			TcpBackClientSocket::OnTimeOut();
		}
		else
		{
			bool ishavetimeout;
			GetSpeed(&ishavetimeout);
			if(ishavetimeout == false)
			{
				AC_INFO("server not slow %s:%d",Getip(),Getport());
				SetSlow(false);
				UnRegisterTimer();
			}
			if(IsSlow())
			{
				if(m_testspeedstream.IsValid())
				{
					for(int i = 0;i < m_testtime;i++)
					{
						if(SendStream(m_testspeedstream) != 0)
							break;
					}
				}
			}
		}
	}
}

#include "yt/log/log.h"
#include "yt/network/reservedata.h"
#include "yt/network/clientsocket.h"

namespace yt
{
	void ReserveData::OnTimeOut()
	{
		/*if(svr)
		{
			svr->AddTime(0);
			AC_ERROR("pack time out = %s",svr->GetDesc());

			if(svr->m_SlowCount++ >= 5)
			{
				AC_ERROR("server slow %s",svr->GetDesc());
				svr->SetSlow(true);
			}
		}*/
		delete this;
	}
}


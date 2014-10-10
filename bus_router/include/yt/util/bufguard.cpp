#include "yt/util/bufguard.h"
#include <stdlib.h>
#include <memory>

namespace yt
{
	BufGuard::BufGuard(char *buf,bool flag) : m_buf(buf),m_flag(flag){}
	BufGuard::~BufGuard()
	{
		if(m_flag) 
			free(m_buf);
	}
}

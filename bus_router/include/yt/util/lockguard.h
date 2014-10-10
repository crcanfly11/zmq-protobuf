#pragma once

#include "lockable.h"

namespace yt
{
/*
	LockableµÄauto_ptr
*/
	class LockGuard
	{
		public:
			LockGuard(Lockable *lock) : m_lock(lock),m_flag(true){}
			~LockGuard(){
				if(m_flag)
					m_lock->Release();
			}
			inline void Unable(){
				m_flag = false;
			}
		private:
			Lockable *m_lock;
			bool m_flag;
	};
}


#pragma once

#include <list>
#include <vector>
#include "yt/util/mutex.h"

using namespace std;

namespace yt
{
	template<class T,class MUTEX = NullMutex>
	class PtrsGuard
	{
		public:
			void Add(T* pt)
			{
				m_mutex.Acquire();
				m_tptrlist.push_back(pt);
				m_mutex.Release();
			}
			~PtrsGuard()
			{
				m_mutex.Acquire();
				for(size_t i = 0;i < m_tptrlist.size();i++)
				{
					delete m_tptrlist[i];
				}
				m_tptrlist.clear();
				m_mutex.Release();
			}
			inline vector<T*>* Get() const{return &m_tptrlist;}
		private:
			vector<T*> m_tptrlist;
			MUTEX m_mutex;
	};
}

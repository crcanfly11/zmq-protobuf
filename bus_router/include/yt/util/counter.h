#pragma once

#include <ext/hash_map>
#include <utility>

namespace yt
{
/*
生成seq的类
*/
	class Counter
	{
		public:
			Counter(int min = 1,int max = 100000000)
				: m_count(min),m_min(min),m_max(max){}
			inline int Get()
			{
				if(m_count >= m_max) m_count = m_min;
				return ++m_count;
			}
		private:
			int m_count;
			int m_min;
			int m_max;
	};
}


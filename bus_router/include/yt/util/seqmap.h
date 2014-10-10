#pragma once

#include <map>
#include <ext/hash_map>
#include <utility>

using namespace std;
using namespace __gnu_cxx;

namespace yt
{
	//key是int型的hash_map的包装，很多地方用到了int型key的hash_map
	template <class T,class Map = hash_map<int,T> >
		class SeqMap
		{
			public:
				typedef Map MAP;
				typedef typename Map::iterator iterator;
				typedef typename Map::const_iterator const_iterator;
			public:
				inline iterator Begin()
				{
					return m_clientmap.begin();
				}
				inline const_iterator Begin() const
				{
					return m_clientmap.begin();
				}
				inline iterator End()
				{
					return m_clientmap.end();
				}
				inline const_iterator End() const
				{
					return m_clientmap.end();	
				}
				void Put(int id,T t)
				{
					//m_clientmap.insert(make_pair(id,t));
					m_clientmap[id] = t;
				}
				T* Get(int id)
				{
					typename Map::iterator it;
					it = m_clientmap.find(id);
					if(it == m_clientmap.end())
						return NULL;
					return &(it->second);
				}
				inline void Del(int id)
				{
					m_clientmap.erase(id);
				}
				inline size_t Size()
				{
					return m_clientmap.size();
				}
				inline void Clear()
				{
					return m_clientmap.clear();
				}
				inline Map* GetMap(){
					return &m_clientmap;
				}
			private:
				Map m_clientmap;
		};
}


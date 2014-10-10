#pragma once

#include "yt/util/rw_mutex.h"
#include <map>

namespace yt
{
	//��д����map,�̰߳�ȫ
	template <class Key,class Type,class Traits = less<Key>,class Allocator=allocator<pair <const Key, Type> > >
	class RWTSMap : public map<Key,Type,Traits,Allocator> , public ThreadRWMutex{};	
}


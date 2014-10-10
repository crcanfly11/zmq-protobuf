#pragma once

#include "yt/util/rw_mutex.h"
#include <map>

namespace yt
{
	//读写锁的map,线程安全
	template <class Key,class Type,class Traits = less<Key>,class Allocator=allocator<pair <const Key, Type> > >
	class RWTSMap : public map<Key,Type,Traits,Allocator> , public ThreadRWMutex{};	
}


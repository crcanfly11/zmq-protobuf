#pragma once

#include <pthread.h>
#include <ext/hash_map>
#include <utility>
#include "yt/util/rw_mutex.h"

typedef void(*CLEANUP_FUNC)(void* p);

using namespace std;
using namespace __gnu_cxx;

namespace yt 
{
	//线程私有数据
	class TSS
	{
		private:
			pthread_key_t key;
		public:
			TSS(CLEANUP_FUNC cleanfunc);
			~TSS();
			void set(void* p);
			void* get();
	};
	/*class TSS2
	{
		public:
			TSS2(CLEANUP_FUNC cleanfunc = NULL) : m_cleanfunc(cleanfunc){}
			~TSS2();
			void set(void* p);
			void* get();
		private:
			hash_map<pthread_t,void*> m_map;
			CLEANUP_FUNC m_cleanfunc;
			ThreadRWMutex m_rwmutex;
	};*/
}


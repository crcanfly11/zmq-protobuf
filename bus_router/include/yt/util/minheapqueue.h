#pragma once
#include"yt/util/minheap.h"
#include"yt/network/eventhandler.h"
using namespace std;
using namespace yt;
namespace yt
{
	class TMMinHeapQueue
	{
		public:
			//MinHeapQueue();
			bool Push(TMEventHandler * handler);
			TMEventHandler * Front();
			TMEventHandler * Pop();
			bool Del(TMEventHandler * handler);
			bool IsEmpty() const;
			void Clear();
		private:
			MinHeap<pair<long ,long > > minheap;
			//TMEventHandler *head;
			multimap<pair<long ,long >,TMEventHandler * > m_map;
	};
}

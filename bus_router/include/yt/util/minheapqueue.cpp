#include"minheapqueue.h"
#include"yt/network/eventhandler.h"
using namespace std;
using namespace yt;
namespace yt
{
	bool TMMinHeapQueue::Push(TMEventHandler * handler)
	{
		pair<long ,long > time(handler->endtime.tv_sec,handler->endtime.tv_usec);
		m_map.insert(make_pair(time,handler));
		return minheap.Insert(time);
	}

	TMEventHandler * TMMinHeapQueue::Pop()
	{
		pair<long ,long > time=minheap.RemoveMin();
		multimap< pair<long ,long >,TMEventHandler * >::iterator it= m_map.find(time);
		if(it!=m_map.end())
		{
			TMEventHandler * handler=it->second;
			m_map.erase(it);
			return handler;
		}
		return NULL;
	}

	TMEventHandler *  TMMinHeapQueue::Front()
	{
		pair<long ,long > time=minheap.GetMin();
		multimap<pair<long ,long >,TMEventHandler * >::iterator it= m_map.find(time);
		if(it!=m_map.end())
		{
			return it->second;
		}
		return NULL;

	}

	bool TMMinHeapQueue::Del(TMEventHandler * handler)
	{
		pair<long ,long > time(handler->endtime.tv_sec,handler->endtime.tv_usec);
		minheap.Del(time);
		multimap<pair<long ,long >,TMEventHandler *>::iterator beg=m_map.lower_bound(time),back=m_map.upper_bound(time);
		while(beg!=back)
		{
			if(beg->second==handler)
			{
				m_map.erase(beg);
				return true;
			}
			else beg++;
		}
		return false;
	}

	bool TMMinHeapQueue::IsEmpty() const
	{
		return minheap.IsEmpty();
	}

	void TMMinHeapQueue::Clear()
	{
		minheap.Clear();
		m_map.clear();
	}
}

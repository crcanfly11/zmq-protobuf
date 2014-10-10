#pragma once

#include <pthread.h>
#include <list>
#include <yt/util/notify.h>
#include "yt/log/log.h"

namespace yt 
{
	//队列类的包装
	template<class T>
		class Queue
		{
			public:
				Queue() : notify(0) {}
				virtual ~Queue() {}
				void SetNotification(Notification* notify) { this->notify = notify; }
				Notification* GetNotification() { return this->notify; }
				int Push(const T& value)
				{
					int ret = PushImp(value);
					if ( ret == 0 && GetNotification() ) {
						GetNotification()->notify();
					}
					return ret;
				}
				int Pop(T& value, bool block = false)
				{
					return PopImp(value, block);
				}
				virtual size_t GetSize() const = 0;
			protected:
				virtual int PushImp(const T& value) = 0;
				virtual int PopImp(T& value, bool block) = 0;
			private:
				Notification* notify;
		};

	template<class T>
		class SyncQueue : public Queue<T>
		{
			public:
				enum { DEFAULT_LIMIT = 1000 };
			public:
				SyncQueue(size_t limit = DEFAULT_LIMIT) 
					: limit_(limit)
					{
						pthread_mutex_init(&mutex, NULL);
						pthread_cond_init(&cond, NULL);
					}
				virtual ~SyncQueue()
				{
					pthread_cond_destroy(&cond);
					pthread_mutex_destroy(&mutex);
				}
				virtual size_t GetSize() const		
				{
					pthread_mutex_lock(&mutex);
					size_t size = queue.size();
					pthread_mutex_unlock(&mutex);
					return size;
				}
				inline void SetLimit(size_t limit){limit_ = limit;}
			protected:
				virtual int PushImp(const T& value)
				{
					pthread_mutex_lock(&mutex);

					if ( queue.size() > limit_ ) {
						pthread_mutex_unlock(&mutex);			
						return -1;
					}
					queue.push_back(value);

					//pthread_cond_broadcast(&cond);
					pthread_cond_signal(&cond);
					pthread_mutex_unlock(&mutex);

					return 0;
				}
				virtual int PopImp(T& value, bool block)		
				{
					pthread_mutex_lock(&mutex);
					while(true)
					{
						if (!queue.empty())
						{
							size_t size = queue.size();;
							if ((size % 100) == 0)
								AC_INFO("Queue Size Point[%lu]\n", size);
							value = queue.front();
							queue.pop_front();
							pthread_mutex_unlock(&mutex);
							return 0;
						}

						if (!block)
						{
							pthread_mutex_unlock(&mutex);
							return -1;
						}

						pthread_cond_wait(&cond, &mutex);
					}
				}
			private:
				mutable pthread_mutex_t mutex;
				mutable pthread_cond_t  cond;
				std::list<T> queue;
				size_t	limit_;
		};
}


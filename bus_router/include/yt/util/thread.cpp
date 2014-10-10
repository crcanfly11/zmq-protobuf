#include <yt/util/thread.h>
#include <pthread.h>
using namespace std;

namespace yt {

	void* __THREAD_FUNC(void* p)
	{
		Thread* thread = static_cast<Thread*>(p);
		thread->Run();
		pthread_exit(0);
		return NULL;
	}

	Thread::Thread()
		: hdl(0)
		{
		}

	int Thread::Start(int detachstate)
	{
		pthread_attr_t attr;
		if ( pthread_attr_init(&attr) != 0 ) {
			return -1;
		}

		/*if ( pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE) != 0 ) {
			return -1;
		}*/
		if ( pthread_attr_setdetachstate(&attr, detachstate) != 0 ) {
			return -1;
		}

		if (pthread_create(&hdl, &attr, __THREAD_FUNC, this) == -1) {
			return -1;
		}

		return 0;
	}

	void Thread::Cancel()
	{
		pthread_cancel(hdl);
	}

	void Thread::Join()
	{
		pthread_join(hdl, NULL);
	}

} // namespace yt


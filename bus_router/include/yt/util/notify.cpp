#include <yt/util/notify.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>

namespace yt 
{
	void SocketNotification::notify()
	{
		::send(m_fd, "A", 1, MSG_DONTWAIT);
	}

	void SignalNotification::notify()
	{
		::raise(m_sig);
	}
}


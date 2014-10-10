#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

namespace yt 
{
	//socket包装,没啥用其实
	class Socket
	{
		private:
			int handle;

		public:
			int GetHandle() const { return handle; }

			void SetHandle(int handle) { this->handle = handle; }

			int Create(int domain, int type, int protocol);

			int ConnectUnixSocket(const char* path, unsigned int usec_timeout);

			int Connect(const char* host, unsigned short port, unsigned int usec_timeout);

			int Connect(const sockaddr* addr, size_t addrlen, unsigned int usec_timeout);

			int BindUnixSocket(const char* path);

			int Bind(const char* host, unsigned short port);

			int Bind(const sockaddr* addr, size_t addrlen);

			int Listen(unsigned int backlog);

			int Accept(Socket& client);

			int SetSendTimeout(unsigned int usec_timeout);

			int SetRecvTimeout(unsigned int usec_timeout);

			int SetSendBufSize(int size);

			int SetRecvBufSize(int size);

			int SetDelay(bool delay);

			int SetBlocking(bool blocking);

			int GetBlocking(bool& blocking);

			int Send(char* buf, size_t buflen);

			int Recv(char* buf, size_t buflen);

			int CanRecv(int sec,int usec = 0);

			void Close();

			static u_int32_t GetMyIp();
	};

} // namespace yt


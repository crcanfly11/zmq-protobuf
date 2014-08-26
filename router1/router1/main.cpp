#include "bus_router.h"

int main(int argc, char *argv[])
{
	void* context = zmq_init(1);
	void* socket = zmq_socket(context, ZMQ_ROUTER);
	zmq_setsockopt(socket, ZMQ_IDENTITY, "router1", 7);

	if(int err = zmq_bind(socket, "tcp://*:11523") == 0)
		cout<< "binded."<< endl;
	else 
		bind_error(err);

	char* addr = new char[64];
	char* msg = new char[128];

	while(1) {
		memset(addr, 0x00, 64);
		memset(msg, 0x00, 128);

		int recv_addr_len = zmq_recv(socket, addr, 64, 0);
		recv_error(recv_addr_len);
		cout<< "recv addr:"<< addr<< ", recv addr len:"<< recv_addr_len<< endl;

		int recv_msg_len = zmq_recv(socket, msg, 128, 0);
		recv_error(recv_msg_len);
		cout<< "recv msg:"<< msg<< ", recv msg len:"<< recv_msg_len<< endl;

		int send_addr_len = zmq_send(socket, addr, recv_addr_len, ZMQ_SNDMORE);
		send_error(send_addr_len);
		cout<< "send addr:"<< addr<< ", send addr len:"<< send_addr_len<< endl;

		int send_msg_len = zmq_send(socket, msg, recv_msg_len, 0);
		send_error(send_msg_len);
		cout<< "send msg:"<< msg<< ", send msg len:"<< send_msg_len<< endl;
		cout<< "-----------------------------------"<< endl;
	}

	//bus_router rt(socket);
	//rt.run();

	system("pause");

	return 0;
};
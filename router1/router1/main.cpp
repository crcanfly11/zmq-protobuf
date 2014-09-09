#include "bus_router.h"

bool end_process = false;

DWORD WINAPI SubAndPub(void *pv)
{
	void *context = zmq_init (1);
	void *xsub = zmq_socket(context, ZMQ_XSUB);
	zmq_bind(xsub, "tcp://*:36001");
	void *xpub = zmq_socket(context, ZMQ_XPUB);
	zmq_bind(xpub, "tcp://*:36002");

	zmq_proxy(xsub, xpub, NULL);

	zmq_close(xsub);
	zmq_close(xpub);
	zmq_ctx_destroy(context);

	return 0;
}

int main(int argc, char *argv[])
{
	void* context = zmq_init(1);

	HANDLE h = CreateThread(NULL,0,SubAndPub,NULL,0,0);

	void* socket = zmq_socket(context, ZMQ_ROUTER);
	//zmq_setsockopt(socket, ZMQ_IDENTITY, "router1", 7);

	if(int err = zmq_bind(socket, "tcp://*:36000") == 0)
		cout<< "binded."<< endl;
	else 
		bind_error(err);

	bus_router rt(socket);
	rt.run();

	zmq_close(socket);
	zmq_ctx_destroy(context);

	system("pause");

	return 0;
};

//test1
//int main(int argc, char *argv[])
//{
//	void* context = zmq_init(1);
//	void* socket = zmq_socket(context, ZMQ_ROUTER);
//	zmq_setsockopt(socket, ZMQ_IDENTITY, "router1", 7);
//
//	if(int err = zmq_bind(socket, "tcp://*:11523") == 0)
//		cout<< "binded."<< endl;
//	else 
//		bind_error(err);
//
//	char* addr = new char[64];
//	char* msg = new char[128];
//
//	while(1) {
//		memset(addr, 0x00, 64);
//		memset(msg, 0x00, 128);
//
//		int recv_addr_len = zmq_recv(socket, addr, 64, 0);
//		recv_error(recv_addr_len);
//		cout<< "recv addr:"<< addr<< ", recv addr len:"<< recv_addr_len<< endl;
//
//		int recv_msg_len = zmq_recv(socket, msg, 128, 0);
//		recv_error(recv_msg_len);
//		cout<< "recv msg:"<< msg<< ", recv msg len:"<< recv_msg_len<< endl;
//
//		int send_addr_len = zmq_send(socket, addr, recv_addr_len, ZMQ_SNDMORE);
//		send_error(send_addr_len);
//		cout<< "send addr:"<< addr<< ", send addr len:"<< send_addr_len<< endl;
//
//		int send_msg_len = zmq_send(socket, msg, recv_msg_len, 0);
//		send_error(send_msg_len);
//		cout<< "send msg:"<< msg<< ", send msg len:"<< send_msg_len<< endl;
//		cout<< "-----------------------------------"<< endl;
//	}
//
//	system("pause");
//
//	return 0;
//};

//test2 心跳

//#define Request_Timeout 2500
//#define Request_Retries 3
//#define Server_EndPoint "tcp://localhost:6666"
//
//int main(void)
//{
//	void* context = zmq_init(1);
//	void* client = zmq_socket(context,ZMQ_REQ);
//	bool continue_retry = true;
//
//	while(continue_retry){
//		zmq_pollitem_t items[] = {{client,0,ZMQ_POLLIN,0}};
//
//		int rc = zmq_poll(items,1,Request_Timeout * ZMQ_POLL_MSEC);
//
//		// 被中断
//		if (rc == -1){
//			break;
//		}
//
//		if (items[0].revents & ZMQ_POLLIN)  {
//			char* reply = zstr_recv(client);
//			// 被中断
//			if (reply == NULL){
//				break;
//			}
//			else if (atoi(reply) == seq){
//				printf("收到正确的回应: %s\n",reply);
//				retry_left = Request_Retries;
//				continue_retry = false;
//				free(reply);
//			}
//			else {
//				printf("错误的回应: %s\n",reply);
//				free(reply);
//			}
//		}
//		else if (--retry_left == 0){
//			printf("服务器好像离线了,放弃\n");
//			break;
//		}
//		// 回应超时,断开连接后重新连接,重发请求
//		else {
//			printf("服务器回应超时,重新连接到服务器...");
//			fflush(stdout);
//			zsocket_destroy(ctx,client);
//
//			client = zsocket_new(ctx,ZMQ_REQ);
//			assert(client);
//			zsocket_connect(client,Server_EndPoint);
//
//			printf("OK\n重发请求: %s...",request);
//			fflush(stdout);
//
//			zstr_send(client,request);
//			printf("OK\n");
//			}
//		}
//	}
//
//	return 0;
//}
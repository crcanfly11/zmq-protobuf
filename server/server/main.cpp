#include <zmq.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

//#include "dzh_bus_interface.pb.h"
#include "zmq_helper.h"

using namespace std;

int main (void)
{
    void *context = zmq_init (1);

    //  与客户端通信的套接字
    void *socket = zmq_socket (context, ZMQ_REP);
	zmq_setsockopt(socket, ZMQ_IDENTITY, "sv1", 3);

	int err;
	//if(err = zmq_bind(socket, "tcp://*:15555") == 0)
	//	cout<< "bind succeed."<< endl;
	//else
	//	error_print(err);
	if(err = zmq_connect(socket, "tcp://localhost:15556") == 0)
		cout<< "connect succeed."<< endl;
	else
		connect_error(err);
   

	while(1)  {	
		//  等待客户端请求
		char* s_recv_msg = s_recv(socket);
		cout<< "recv:"<< recv<< endl;
		free(recv);
		
		//应答
		char* send = "hello world.";
		s_send_msg(socket, send);
		cout<< "send:"<< send<< endl;

		cout<< "--------------------------------------"<< endl;
	}

    //  程序不会运行到这里，以下只是演示我们应该如何结束
    zmq_close (socket);
    zmq_term (context);

	system("pause");

    return 0;
}
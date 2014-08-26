#include <zmq.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

#include "zmq_helper.h"
#include "dzh_bus_interface.pb.h"

using namespace std;

int main(void)
{
	void *context = zmq_init(1);
	void *router = zmq_socket (context, ZMQ_ROUTER);
	void *dealer  = zmq_socket (context, ZMQ_DEALER);
	zmq_setsockopt(router, ZMQ_IDENTITY, "router", 6);
	zmq_setsockopt(dealer, ZMQ_IDENTITY, "dealer", 6);
	zmq_bind(router, "tcp://*:15555");
	zmq_bind(dealer, "tcp://*:15556");

    //初始化轮询集合
    zmq_pollitem_t items [] = {
        { router, 0, ZMQ_POLLIN, 0 },
        { dealer, 0, ZMQ_POLLIN, 0 }
    };
    //在套接字间转发消息
    while (1) {
		char* recv_addr; 
		char* recv_empty;
		char* recv_data;

        zmq_poll (items, 2, -1);
        if (items [0].revents & ZMQ_POLLIN) {
			//接收
			recv_addr = s_recv_msg(router);
			cout<< "router recv addr:"<< recv_addr<< endl;
			
			recv_empty = s_recv_msg(router);
			cout<< "router recv empty:"<< recv_empty<< endl;
			
			recv_data = s_recv_msg(router);
			cout<< "router recv data:"<< recv_data<< endl;
			
			//转发
			//单server的时候写server还是dealer都能发的过去
			char* dealer_str = "sv1";   //sv1   dealer
			s_sendmore_msg(dealer, dealer_str);
			cout<< "send dealer addr_rep:"<< dealer_str<< endl;
 
			s_sendmore_msg(dealer, recv_empty);
			cout<< "send dealer empty_rep:"<< recv_empty<< endl;

			//数据帧
			s_send_msg(dealer, recv_data);
			cout<< "send dealer data_rep:"<< recv_data<< endl;
			cout<< "-------------------------------------"<< endl;
        }
        if (items [1].revents & ZMQ_POLLIN) {
			//接收
			recv_addr = s_recv_msg(dealer);
			cout<< "dealer recv addr:"<< recv_addr<< endl;

			recv_empty = s_recv_msg(dealer);
			cout<< "dealer recv empty:"<< recv_empty<< endl;

			recv_data = s_recv_msg(dealer);
			cout<< "dealer recv data:"<< recv_data<< endl;

			//转发
			//信封要指明 发到哪个req上，发到router上 转不出去
			char* dealer_str = "ch1";   //router   ch1
			s_sendmore_msg(router, dealer_str);
			cout<< "send router addr_rep:"<< dealer_str<< endl;

			s_sendmore_msg(router, recv_empty);
			cout<< "send router empty_rep:"<< recv_empty<< endl;

			//数据帧
			s_send_msg(router, recv_data);
			cout<< "send router data_rep:"<< recv_data<< endl;

			cout<< "-------------------------------------"<< endl;
        }
		free(recv_addr);
		free(recv_empty);
		free(recv_data);
    }

	//clear
	zmq_close (router);
	zmq_close (dealer);
    zmq_term (context);

	system("pause");

    return 0;
}

//int main (void)
//{
//    void *context = zmq_init (1);
//
//    //  客户端套接字
//    void *frontend = zmq_socket (context, ZMQ_ROUTER);
//    zmq_bind (frontend, "tcp://*:15555");
//
//    //  服务端套接字
//    void *backend = zmq_socket (context, ZMQ_DEALER);
//    zmq_bind (backend, "tcp://*:15556");
//
//    //  启动内置装置
//    zmq_device (ZMQ_QUEUE, frontend, backend);
//
//    //  程序不会运行到这里
//    zmq_close (frontend);
//    zmq_close (backend);
//    zmq_term (context);
//    return 0;
//}
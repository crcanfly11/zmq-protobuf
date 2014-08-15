//
//  Hello World 服务端
//  绑定一个REP套接字至tcp://*:5555
//  从客户端接收Hello，并应答World
//
#include <zmq.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include "addressbook.pb.h"

using namespace std;

int main (void)
{
    void *context = zmq_init (1);

    //  与客户端通信的套接字
    void *responder = zmq_socket (context, ZMQ_REP);
    zmq_bind (responder, "tcp://*:5555");

	while(1)  {	
		//  等待客户端请求
		zmq_msg_t request;
		zmq_msg_init (&request);
		zmq_recv (responder, &request, 5000, 0);   //ZMQ_NOBLOCK

		string bufRec((const char*)zmq_msg_data(&request));
		tutorial::AddressBook msgAddressBookRec; 
		tutorial::Person* msgPersonRec = msgAddressBookRec.add_person();
		msgAddressBookRec.ParseFromString(bufRec);

		std::cout<< "id:"<< msgPersonRec->id()<< std::endl;
		std::cout<< "name:"<< msgPersonRec->name()<< std::endl;
     
		zmq_msg_close (&request);
	}

    //  程序不会运行到这里，以下只是演示我们应该如何结束
    zmq_close (responder);
    zmq_term (context);

	system("pause");

    return 0;
}
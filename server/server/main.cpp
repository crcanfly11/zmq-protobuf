//
//  Hello World �����
//  ��һ��REP�׽�����tcp://*:5555
//  �ӿͻ��˽���Hello����Ӧ��World
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

    //  ��ͻ���ͨ�ŵ��׽���
    void *responder = zmq_socket (context, ZMQ_REP);
    zmq_bind (responder, "tcp://*:5555");

	while(1)  {	
		//  �ȴ��ͻ�������
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

    //  ���򲻻����е��������ֻ����ʾ����Ӧ����ν���
    zmq_close (responder);
    zmq_term (context);

	system("pause");

    return 0;
}
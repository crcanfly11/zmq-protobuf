//
//  Hello World 客户端
//  连接REQ套接字至 tcp://localhost:5555
//  发送Hello给服务端，并接收World
//
#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <iostream>	
#include <fstream>
#include "addressbook.pb.h"

using namespace std;

void write()
{
	tutorial::AddressBook msgAddressBook; 
	tutorial::Person* msgPerson = msgAddressBook.add_person();
	msgPerson->set_id(112);
	msgPerson->set_name("hello");
	    
	// Write the new address book back to disk. 
	fstream output("./log.pb", ios::out | ios::trunc | ios::binary); 
        
	if (!msgAddressBook.SerializeToOstream(&output)) { 
		cerr << "Failed to write msg." << endl; 
	}         
	
	cout<<"write succeed."<<endl;
}

void read()
{
	tutorial::AddressBook msgAddressBook; 
	tutorial::Person* msgPerson = msgAddressBook.add_person();
 
	fstream input("./log", ios::in | ios::binary); 
	if (!msgAddressBook.ParseFromIstream(&input)) { 
		cerr << "Failed to parse address book." << endl; 
	} 
  
	cout << msgPerson->id() << endl; 
	cout << msgPerson->name() << endl; 
}

int main (void)
{
    void *context = zmq_init (1);

    //  连接至服务端的套接字
    printf ("正在连接至hello world服务端...\n");
    void *requester = zmq_socket (context, ZMQ_REQ);
    zmq_connect (requester, "tcp://localhost:5555");

	//组建数据流
	tutorial::AddressBook msgAddressBook; 
	tutorial::Person* msgPerson = msgAddressBook.add_person();
	msgPerson->set_id(321);
	msgPerson->set_name("protobuf111");

	std::string buf;
	msgAddressBook.SerializeToString(&buf);

    zmq_msg_t request;
	zmq_msg_init(&request);

	memcpy (zmq_msg_data (&request), (const void*)buf.c_str(), buf.size());
    printf ("正在发送...\n");
    int s = zmq_send (requester, &request,  buf.size(), 0);

	if(s==0)
		std::cout<< "Succeed."<< std::endl;
	else if(s == EAGAIN) 
		std::cout<< "Non-blocking mode was requested and the message cannot be sent at the moment."<< std::endl;
	else if(s == ENOTSUP) 
		std::cout<< "The zmq_send() operation is not supported by this socket type."<< std::endl;
	else if(s == EFSM) 
		std::cout<< "The zmq_send() operation cannot be performed on this socket at the moment due to the socket not being in the appropriate state. "<< std::endl;
	else if(s == ETERM) 
		std::cout<< "The ZMQ context associated with the specified socket was terminated."<< std::endl;
	else if(s == ENOTSOCK) 
		std::cout<< "The provided socket was invalid."<< std::endl;
	else if(s == EINTR) 
		std::cout<< "The operation was interrupted by delivery of a signal before the message was sent."<< std::endl;
	else if(s == EFAULT) 
		std::cout<< "Invalid message."<< std::endl;

    zmq_msg_close (&request);

    zmq_close (requester);
    zmq_term (context);

	system("pause");

    return 0;
}
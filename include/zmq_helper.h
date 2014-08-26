#ifndef __ZMQ_HELPER_HPP__
#define __ZMQ_HELPER_HPP__

#include <zmq.h>
#include <assert.h>
#include <iostream>
#include <iomanip>
#include <string>

#define BUF_MAX_LEN 256

using namespace std;

static char* s_recv_msg(void *socket) 
{
    zmq_msg_t message;
    zmq_msg_init(&message);
    zmq_recvmsg(socket, &message, 0);
    int size = zmq_msg_size(&message);
    char *string = (char*)malloc(size + 1);
    memcpy(string, zmq_msg_data(&message), size);
    zmq_msg_close(&message);
    string[size] = 0;
    return string;
};

static char* s_recv(void *socket) 
{
	char* pbuf = new char[BUF_MAX_LEN];
	memset(pbuf, 0x00, BUF_MAX_LEN);
    zmq_recv(socket, pbuf, BUF_MAX_LEN, 0);

    return pbuf;
};

static int s_send_msg(void *socket, char *string) 
{
    int rc;
	int size = strlen(string);
    zmq_msg_t message;
    zmq_msg_init_size(&message, size);
    memcpy(zmq_msg_data(&message), string, size);
    rc = zmq_sendmsg(socket, &message, 0);
    zmq_msg_close(&message);
    return (rc);
};

static int s_send(void *socket, char *pbuf) 
{
    int rc;
    rc = zmq_send(socket, pbuf, BUF_MAX_LEN, 0);
    return (rc);
};

static int s_sendmore_msg(void *socket, char *string) 
{
    int rc;
    zmq_msg_t message;
    zmq_msg_init_size(&message, strlen (string));
    memcpy(zmq_msg_data (&message), string, strlen (string));
	rc = zmq_sendmsg(socket, &message, ZMQ_SNDMORE);
    zmq_msg_close(&message);
    return (rc);
};

static int s_sendmore(void *socket, char *pbuf) 
{
    int rc;
    rc = zmq_send(socket, pbuf, BUF_MAX_LEN, ZMQ_SNDMORE);
    return (rc);
};

static void bind_error(int c)
{
	if(c == EINVAL)
		cout<< "The endpoint supplied is invalid."<< endl;
	else if(c == EPROTONOSUPPORT)
		cout<< "The requested transport protocol is not supported."<< endl;
	else if(c == ENOCOMPATPROTO)
		cout<< "The requested transport protocol is not compatible with the socket type."<< endl;
	else if(c == EADDRINUSE)
		cout<< "The requested address is already in use."<< endl;
	else if(c == EADDRNOTAVAIL)
		cout<< "The requested address was not local."<< endl;
	else if(c == ENODEV)
		cout<< "The requested address specifies a nonexistent interface."<< endl;
	else if(c == ETERM)
		cout<< "The ZMQ context associated with the specified socket was terminated."<< endl;
	else if(c == ENOTSOCK)
		cout<< "The provided socket was invalid."<< endl;    
	else if(c == EMTHREAD)
		cout<< "No I/O thread is available to accomplish the task."<< endl; 
};

static void connect_error(int c)
{
	if(c == EINVAL)
		cout<< "The endpoint supplied is invalid."<< endl;
	else if(c == EPROTONOSUPPORT)
		cout<< "The requested transport protocol is not supported."<< endl;
	else if(c == ENOCOMPATPROTO)
		cout<< "The requested transport protocol is not compatible with the socket type."<< endl;
	else if(c == ETERM)
		cout<< "The ZMQ context associated with the specified socket was terminated."<< endl;
	else if(c == ENOTSOCK)
		cout<< "The provided socket was invalid."<< endl;    
	else if(c == EMTHREAD)
		cout<< "No I/O thread is available to accomplish the task."<< endl; 
};


static void send_error(int c)
{
	if(c == EAGAIN)
		cout<< "Non-blocking mode was requested and the message cannot be sent at the moment."<< endl;
	else if(c == ENOTSUP)
		cout<< "The zmq_send() operation is not supported by this socket type."<< endl;
	else if(c == EFSM)
		cout<< "The zmq_send() operation cannot be performed on this socket at the moment due to the socket not being in the appropriate state. This error may occur with socket types that switch between several states, such as ZMQ_REP. See the messaging patterns section of zmq_socket(3) for more information."<< endl;
	else if(c == ETERM)
		cout<< "The ZMQ context associated with the specified socket was terminated."<< endl;
	else if(c == ENOTSOCK)
		cout<< "The provided socket was invalid."<< endl;
	else if(c == EINTR)
		cout<< "The operation was interrupted by delivery of a signal before the message was sent."<< endl;    
	else if(c == EFAULT)
		cout<< "Invalid message."<< endl; 
};

static void recv_error(int c)
{
	if(c == EAGAIN)
		cout<< "Non-blocking mode was requested and the message cannot be sent at the moment."<< endl;
	else if(c == ENOTSUP)
		cout<< "The zmq_recv() operation is not supported by this socket type."<< endl;
	else if(c == EFSM)
		cout<< "The zmq_recv() operation cannot be performed on this socket at the moment due to the socket not being in the appropriate state. This error may occur with socket types that switch between several states, such as ZMQ_REP. See the messaging patterns section of zmq_socket(3) for more information."<< endl;
	else if(c == ETERM)
		cout<< "The ZMQ context associated with the specified socket was terminated."<< endl;
	else if(c == ENOTSOCK)
		cout<< "The provided socket was invalid."<< endl;
	else if(c == EINTR)
		cout<< "The operation was interrupted by delivery of a signal before a message was available."<< endl;    
	else if(c == EFAULT)
		cout<< "The message passed to the function was invalid."<< endl; 
};

#endif
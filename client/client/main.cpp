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
#include "zmq_helper.h"
#include "dzh_bus_interface.pb.h"

using namespace std;
using namespace google;
using namespace protobuf;

//int main (int argc, char *argv[])
//{
//    void *context = zmq_init (1);
//
//    //  连接至服务端的套接字
//    printf ("正在连接至服务端...\n");
//    void *requester = zmq_socket (context, ZMQ_DEALER);  
//	zmq_setsockopt(requester, ZMQ_IDENTITY, "dealer1", 7);
//
//	int err;
//	if(err = zmq_connect(requester, "tcp://localhost:15558") == 0)
//		cout<< "connect succeed."<< endl;
//	else
//		connect_error(err);
//	
//	while(1) {
//
//		srand((unsigned int)time(NULL));
//		int data = rand()%1000;
//		char buf[4];
//		sprintf(buf, "%d", data);
//		buf[3] = 0;
//		zmq_send(requester, buf, 3, 0);
//		cout<< "send buf:"<< buf<< endl;
//
//		char msg[1024];
//		int recv_size = zmq_recv(requester,msg,1024, 0);
//		msg[recv_size] = 0;
//
//		cout<< "recv :"<< msg<< endl;
//		Sleep(1000);
//	}
//
//	system("pause");
//
//	return 0;
//}

//router 转发测试程序
struct request_string_data
{
	unsigned int command_id;
	bool has_serviceNo;
	vector<string> serviceName_vec;
	unsigned int serviceName_cnt;
};

void split_commond(std::string str, request_string_data& rd)
{
	if(str.find("reply/bus/login") != string::npos)
		rd.command_id = 1;
	else if(str.find("reply/bus/logout") != string::npos)
		rd.command_id = 2;
	else 
		rd.command_id = 0;
};

int main (int argc, char *argv[])
{
	void *context = zmq_init (1);

    //  连接至服务端的套接字
    printf ("正在连接至服务端...\n");
    void *requester = zmq_socket (context, ZMQ_DEALER);  
	//zmq_setsockopt(requester, ZMQ_IDENTITY, "ch1", 3);

	int err;
	if(err = zmq_connect(requester, "tcp://10.15.89.122:36000") == 0)
	//if(err = zmq_connect(requester, "tcp://localhost:36000") == 0)
		cout<< "connect succeed."<< endl;
	else
		connect_error(err);

	cout<< "------start-------"<< endl;

	dzh_bus_interface::AnyReq lg;
	string str = "bus/login";
	lg.set_allocated_url(&str);

	std::string lg_buf; 	
	lg.SerializeToString(&lg_buf);

	dzh_bus_interface::Bus_Head bh;
	bh.set_servicename("bus/login");
	bh.set_requestid(10001);
	bh.set_allocated_data(&lg_buf);

	int bh_size = bh.ByteSize();
	char* bh_buf = new char[bh_size];
	memset(bh_buf, 0x00, bh_size);
	bh.SerializeToArray(bh_buf, bh_size);
	
	zmq_send(requester, bh_buf, bh_size, 0);

	cout<< "------send login req------"<< endl;
	delete[] bh_buf;
	lg.release_url();
	bh.release_data();

	//应答
	cout<< "------ recving login msg`````-------"<< endl;

	char addr[256]; 
	char msg[1024];

	memset(addr, 0x00, sizeof(addr));
	memset(msg, 0x00, sizeof(msg));

	int recv_size = zmq_recv(requester,msg,1024, 0);

	dzh_bus_interface::Bus_Head rep_bh;
	rep_bh.ParseFromArray(msg, recv_size);

	request_string_data rd;
	split_commond(rep_bh.servicename(), rd);

	if(rd.command_id == 1) {
		dzh_bus_interface::LoginRsp lgin_rsp;
		lgin_rsp.ParseFromString(rep_bh.data());
		dzh_bus_interface::RspInfo rspinfo = lgin_rsp.rspinfo();
		cout<< "recv login msg:"<< rspinfo.rspdesc()<< endl;
		rspinfo.release_rspdesc();
		lgin_rsp.release_rspinfo();
	}
	else {
		dzh_bus_interface::RspInfo rspinfo;
		rspinfo.ParseFromString(rep_bh.data());
		cout<< "recv login msg:"<< rspinfo.rspdesc()<< endl;
		rspinfo.release_rspdesc();
	}
	
	rep_bh.release_data();		

	//Sleep(1000*5);

	//转发100请求
	dzh_bus_interface::Bus_Head bhreq;
	bhreq.set_servicename("a");
	bhreq.set_requestid(2301);

	int bhreq_size = bhreq.ByteSize();
	char* bhreq_buf = new char[bhreq_size];
	memset(bhreq_buf, 0x00, bhreq_size);
	bhreq.SerializeToArray(bhreq_buf, bhreq_size);
	zmq_send(requester, bhreq_buf, bhreq_size, 0);

	cout<< "-----send 100 req.-------"<< endl;
	bhreq.release_data();

	//应答
	//while(1) {
		cout<< "------ recving 100 rep`````-------"<< endl;
		memset(msg, 0x00, sizeof(msg));
		int recv_size100 = zmq_recv(requester,msg,1024, 0);
		dzh_bus_interface::Bus_Head rep_bh100;
		rep_bh100.ParseFromArray(msg, recv_size100);

		if(strcmp(rep_bh100.servicename().c_str(), "a") == 0) 
			cout<< "recv a:"<< rep_bh100.endflag()<< endl;
		else if(strcmp(rep_bh100.servicename().c_str(), "b") == 0) 
			cout<< "recv b:"<< rep_bh100.endflag()<< endl;
		else if(strcmp(rep_bh100.servicename().c_str(), "c") == 0) 
			cout<< "recv c:"<< rep_bh100.endflag()<< endl;
		else {
			dzh_bus_interface::RspInfo rsp;
			rsp.ParseFromString(rep_bh100.data());	
			cout<< "error:"<< rsp.rspdesc()<< endl;
			//break;
		}

		rep_bh100.release_data();	
	//}

	cout<< "-------end--------"<< endl;

    zmq_close (requester);
    zmq_term (context);

	system("pause");

    return 0;
}

////ZMQ_REQ 模式测试程序
//int main (void)
//{
//    void *context = zmq_init (1);
//
//    //  连接至服务端的套接字
//    printf ("正在连接至服务端...\n");
//    void *requester = zmq_socket (context, ZMQ_REQ);  
//	zmq_setsockopt(requester, ZMQ_IDENTITY, "ch1", 3);
//
//	int err;
//	if(err = zmq_connect(requester, "tcp://10.15.89.122:15557") == 0)
//		cout<< "connect succeed."<< endl;
//	else 
//		cout<< "connect error."<< endl;
//
//	char* send = "hello1234567";
//	s_send_msg(requester, send);
//	cout<< "send:"<< send<< endl;
//
//	//应答
//	char* recv1 = s_recv_msg(requester);
//	cout<< "recv1:"<< recv1<< endl;
//	free(recv1);
//
//	char* recv2 = s_recv_msg(requester);
//	cout<< "recv2:"<< recv2<< endl;
//	free(recv2);
//
//	char* recv3 = s_recv_msg(requester);
//	cout<< "recv3:"<< recv3<< endl;
//	free(recv3);
//
//    zmq_close (requester);
//    zmq_term (context);
//
//	system("pause");
//
//    return 0;
//}
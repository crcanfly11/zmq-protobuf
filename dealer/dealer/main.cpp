#include <zmq.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

#include "zmq_helper.h"
#include "dzh_bus_interface.pb.h"

using namespace std;
using namespace google;
using namespace protobuf;

//router dealer模式 不需要传0帧
//提供 service 100 200 
int main(int argc, char *argv[])
{
	void *context = zmq_init(1);
	void *dealer = zmq_socket (context, ZMQ_DEALER);
	zmq_setsockopt(dealer, ZMQ_IDENTITY, "dealer1", 7);

	int err;
	if(err = zmq_connect(dealer, "tcp://localhost:11523") == 0)
		cout<< "connected."<< endl;
	else 
		bind_error(err);

	cout<< "------start-------"<< endl;

	dzh_bus_interface::Bus_Head bh;
	bh.set_bodytype(1);
	bh.set_requestid(100);
	bh.set_endflag(3);

	dzh_bus_interface::LoginReq lg;
	lg.add_serviceno(100);
	lg.add_serviceno(200);

	dzh_bus_interface::Body b;
	b.set_allocated_loginreq(&lg);

	bh.set_allocated_body(&b);

	int size = bh.ByteSize();
	char* buf = new char[size];
	memset(buf, 0x00, size);
	bh.SerializeToArray(buf, size);
	
	zmq_send(dealer, buf, size, 0);
	cout<< "--------send login req--------"<< endl;
	delete[] buf;
	b.release_loginreq();
	bh.release_body();

	cout<< "------ recving login msg`````-------"<< endl;

	char addr[256];
	char msg[1024];

	memset(addr, 0x00, sizeof(addr));
	memset(msg, 0x00, sizeof(msg));

	int recv_size = zmq_recv(dealer,msg,1024, 0);

	dzh_bus_interface::Bus_Head rep_bh;
	rep_bh.ParseFromArray(msg, recv_size);
	dzh_bus_interface::RspInfo rep = rep_bh.body().loginrsp().rspinfo();
	cout<< "recv login msg:"<< rep.rspdesc()<< endl;
	rep.release_rspdesc();
	rep_bh.release_body();

	cout<< "--------begin server--------"<< endl;
	while(1) {
		memset(msg, 0x00, sizeof(msg));
		int size1 = zmq_recv(dealer,msg,sizeof(msg), 0);
		dzh_bus_interface::Bus_Head bh;
		bh.ParseFromArray(msg, size1);

		if(bh.bodytype() == 100) {
			bh.set_endflag(11111);
		}
		else if(bh.bodytype() == 200) {
			bh.set_endflag(22222);
		}

		memset(msg, 0x00, sizeof(msg));
		int buf_si = bh.ByteSize();
		bh.SerializeToArray(msg, buf_si);

		zmq_send(dealer, msg, buf_si, 0);

		cout<< "send end flag rep:"<< bh.endflag()<< endl;
	}

	cout<< "---------end----------"<< endl;

    zmq_close (dealer);
    zmq_term (context);

	system("pause");
};

//int main(int argc, char *argv[])
//{
//    if (argc < 2) {
//        printf("input dealer name and port.");
//        exit(EXIT_FAILURE);
//    }
//
//	void *context = zmq_init(1);
//	void *dealer = zmq_socket(context, ZMQ_DEALER);
//	//zmq_setsockopt(dealer, ZMQ_IDENTITY, argv[1], strlen(argv[1]));
//
//	//char* bind_addr = (char*)malloc(100);
//	//sprintf(bind_addr, "tcp://*:%s", argv[2]);
//	int err;
//	if(err = zmq_connect(dealer, "tcp://localhost:11523") == 0)
//		cout<< "connected."<< endl;
//	else 
//		connect_error(err);
//
//	int a = 0;
//	while(1) {
//		s_send(dealer, argv[2]);
//		cout<< "send:"<< argv[2]<< endl;
//
//		char* data = s_recv_msg(dealer);
//		cout<< "recv data:"<< data<< endl;	
//		cout<< "-----------------------------"<< endl;
//
//		free(data);
//
//		if(++a > atol(argv[3]))
//			break;
//	}
//
//    zmq_close (dealer);
//    zmq_term (context);
//
//	system("pause");
//};

//void main()
//{
//	void *context = zmq_init(1);
//	void *dealer = zmq_socket(context, ZMQ_DEALER);
//	//zmq_setsockopt(dealer, ZMQ_IDENTITY, "dealer1", 7);
//
//	if(int err = zmq_connect(dealer, "tcp://localhost:11523") == 0)
//		cout<< "connected."<< endl;
//	else 
//		connect_error(err);
//
//	char addr[256];
//	char msg[256];
//
//	while(1) {
//		memset(addr, 0x00, sizeof(addr));
//		memset(msg, 0x00, sizeof(msg));
//
//		zmq_send(dealer,"cccc", 4, 0);
//		cout<< "send: cccc"<< endl;
//
//		//zmq_recv(dealer,addr,sizeof(addr)-1, 0);
//		zmq_recv(dealer,msg,sizeof(msg), 0);
//
//		cout<< "recv addr:"<< addr<< endl;
//		cout<< "recv msg:"<< msg<< endl;
//		cout<< "-----------------------------"<< endl;
//	}
//}

//int main(int argc,char* argv[]){
//    void * context = zmq_init(1);
//    void* client = zmq_socket(context,ZMQ_ROUTER);
//	zmq_setsockopt (client, ZMQ_IDENTITY, "router2", 7);
//	
//	zmq_connect(client,"tcp://localhost:11523");
//
//
//	char msg[256];
//	char addr[256];
//	  
//    for(int idx = 0; idx < 10000; ++idx){
//		zmq_send(client,"router1",7,ZMQ_SNDMORE);
//        zmq_send(client,"rtor",4,0);
//
//		memset(addr,0x00,sizeof(addr));
//		zmq_recv(client,addr,sizeof(addr)-1,0);
//		memset(msg,0x00,sizeof(msg));
//		zmq_recv(client,msg,sizeof(msg)-1,0);
//
//		cout<< "recv addr:"<< addr<< endl;
//        cout<< "recv msg:"<< msg<< endl;
//    }
//    zmq_close(client);
//    zmq_ctx_destroy(context);
//    return 0;
//}
#include <zmq.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

#include "zmq_helper.h"
#include "dzh_bus_interface.pb.h"

using namespace std;
using namespace google;
using namespace protobuf;

//timer test
//#include <Windows.h>
//#include <Mmsystem.h>
//#pragma comment(lib, "Winmm.lib")
//
//int t = 0;
//
//void WINAPI onTimeFunc(UINT wTimerID, UINT msg,DWORD dwUser,DWORD dwl,DWORD dw2);
//void WINAPI onTimeFunc1(UINT wTimerID, UINT msg,DWORD dwUser,DWORD dwl,DWORD dw2);
//
//int main(int argc, char *argv[]) 
//{
//    MMRESULT timer_id,timer_id1;
//    int n = 1;
//    timer_id = timeSetEvent(5000, 1, (LPTIMECALLBACK)onTimeFunc, DWORD(1), TIME_PERIODIC);
//	timer_id1 = timeSetEvent(1000, 1, (LPTIMECALLBACK)onTimeFunc1, DWORD(1), TIME_PERIODIC);
//    if(NULL == timer_id)
//    {
//        printf("timeSetEvent() failed with error %d\n", GetLastError());
//        return 0;
//    }
//
//	Sleep(2000);
//
//    while(n<50)
//    {
//        printf("Hello World");
//        Sleep(2000);
//        n++;
//    }
//    timeKillEvent(timer_id);        //释放定时器
//    return 1;	
//};
//
//void WINAPI onTimeFunc(UINT wTimerID, UINT msg,DWORD dwUser,DWORD dwl,DWORD dw2)
//{
//    printf("time out");
//    return;
//}
//
//void WINAPI onTimeFunc1(UINT wTimerID, UINT msg,DWORD dwUser,DWORD dwl,DWORD dw2)
//{
//	t++;
//	printf("\nID:%d  ", t);
//}


//router dealer模式 不需要传0帧
//提供 service 100 200 
int main(int argc, char *argv[])
{
	void *context = zmq_init(1);
	void *dealer = zmq_socket (context, ZMQ_DEALER);
	zmq_setsockopt(dealer, ZMQ_IDENTITY, "dealer1", 7);

	int err;
	//if(err = zmq_connect(dealer, "tcp://10.15.89.122:36000") == 0)
	if(err = zmq_connect(dealer, "tcp://localhost:36000") == 0)
		cout<< "connected."<< endl;
	else 
		bind_error(err);

	cout<< "------start-------"<< endl;

	dzh_bus_interface::LoginReq lg;
	lg.add_serviceno(100);
	lg.add_serviceno(200);
	lg.add_serviceno(300);

	std::string lg_buf; 	
	lg.SerializeToString(&lg_buf);

	dzh_bus_interface::Bus_Head bh;
	bh.set_command(1);
	bh.set_requestid(10001);
	bh.set_allocated_data(&lg_buf);

	int bh_size = bh.ByteSize();
	char* bh_buf = new char[bh_size];
	memset(bh_buf, 0x00, bh_size);
	bh.SerializeToArray(bh_buf, bh_size);
	
	zmq_send(dealer, bh_buf, bh_size, 0);

	cout<< "--------send login req--------"<< endl;
	delete[] bh_buf;
	bh.release_data();

	cout<< "------ recving login msg`````-------"<< endl;

	char addr[256];
	char msg[1024];

	memset(addr, 0x00, sizeof(addr));
	memset(msg, 0x00, sizeof(msg));

	int recv_size = zmq_recv(dealer,msg,1024, 0);

	dzh_bus_interface::Bus_Head rep_bh;
	rep_bh.ParseFromArray(msg, recv_size);

	if(rep_bh.command() == 2) {
		dzh_bus_interface::LoginRsp lgin_rsp;
		lgin_rsp.ParseFromString(rep_bh.data());
		cout<< "recv login rsp msg:"<< lgin_rsp.rspinfo().rspdesc()<< endl;
		lgin_rsp.release_rspinfo();
	}
	else {
		dzh_bus_interface::RspInfo rspinfo;
		rspinfo.ParseFromString(rep_bh.data());
		cout<< "recv error msg:"<< rspinfo.rspdesc()<< endl;
		rspinfo.release_rspdesc();
		return 0;
	}
	rep_bh.release_data();		

	cout<< "--------begin server--------"<< endl;
	while(1) {
		memset(msg, 0x00, sizeof(msg));
		int size1 = zmq_recv(dealer,msg,sizeof(msg), 0);
		dzh_bus_interface::Bus_Head bh;
		bh.ParseFromArray(msg, size1);

		if(bh.serviceno() == 100) {
			bh.set_endflag(11111);
		}
		else if(bh.serviceno() == 200) {
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
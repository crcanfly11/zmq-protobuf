#pragma once

#include <zmq.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <string.h>
#include <iostream>

//#include "dzh_bus_interface.pb.h"
#include "dzhbus.pb.h"
#include "zmq_helper.h"

extern bool end_process;

using namespace std;
using namespace google;
using namespace protobuf;
using namespace dzhyun;

const unsigned int ADDRESS_MAX_LENGTH = 64;
const unsigned int MESSAGE_MAX_LENGTH = 1024*1024*1024;
const unsigned int REQUEST_TIMEOUT = 1000;
const unsigned int HEARTBEAT_TIMEOUT = 3600;
const unsigned int REPLAY_WAIT_TIMEOUT = 5;

struct addr_data
{
	std::string addr;
	unsigned int addr_length;
	unsigned int ServerNo;
	long reset_time;
	long alive_time;
	unsigned __int64 requestID;
	vector<string> ServiceName_vector;
};

struct request_string_data
{
	unsigned int command_id;
	vector<string> serviceName_vec;
	unsigned int serviceName_cnt;
};


typedef pair<string, addr_data> addr_pair;
typedef pair<string, unsigned __int64> requestID_pair;
typedef pair<long, unsigned __int64> timer_pair;
typedef pair<long, string> heartbeat_pair;

struct reset_timer_data
{
	requestID_pair req_requestID_pair;
	unsigned int times;
};

typedef map<string, addr_data> addr_map;
typedef map<heartbeat_pair, string> heartbeat_map;
typedef map<requestID_pair, addr_pair> requestID_map;
typedef map<string, addr_map> service_Name_map;
typedef map<timer_pair, reset_timer_data> timer_map;

//typedef map<unsigned __int64, addr_pair> serverNo_map;

class bus_router
{
	addr_map addr_map_;
	heartbeat_map heartbeat_map_;
	//serverNo_map serverNo_map_;
	service_Name_map service_Name_map_;   
	requestID_map requestID_map_;   //管理应答dealer的地址和router的requestID
	timer_map timer_map_;
	
	void* router_socket_;
	char* p_addr_;
	char* p_msg_; 

	unsigned int addr_length_;
	unsigned int msg_length_;
	unsigned __int64 requestID_;

	long time_;

public:
	bus_router();
	bus_router(void* socket);
	~bus_router();

	void run();

private:
	void router_loginReq(BusHead* bus, request_string_data* rd);
	void router_loginRsp(BusHead* bus, request_string_data* rd);
	void router_logoutReq(BusHead* bus);
	void router_logoutRsp(BusHead* bus);
	void router_heartbeatReq(BusHead* bus);
	void router_heartbeatRsp(BusHead* bus);
	void router_other_dealer(BusHead* bus);
	void router_timeoutRsp(addr_pair& pair);
	void reset_map(BusHead* bus);

	void send_msg(void* addr, unsigned int addr_len, void* msg, unsigned int msg_len);
	void send_msg(void* addr, unsigned int addr_len, BusHead* bus);
	void send_error_msg_back(BusHead* bus, char* msg);
	
	int find_address_map(char* addr);
	int compare_addr(service_Name_map::iterator iter_servieName);
	int compare_len(unsigned __int64 len_recv, unsigned __int64 len_local);

	void add_requestID_map(string addr, unsigned __int64 requestID);
	void recycling_watcher();
	void recycling_heartbeat();
	
	void split_command(std::string str, request_string_data& rd);
	void split_anyreq(std::string str, request_string_data& rd);

	void clear();

	void print_map();
};

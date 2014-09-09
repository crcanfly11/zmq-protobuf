#pragma once

#include <zmq.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <time.h>

#include "dzh_bus_interface.pb.h"
#include "zmq_helper.h"

extern bool end_process;

using namespace std;
using namespace google;
using namespace protobuf;

const unsigned int ADDRESS_MAX_LENGTH = 64;
const unsigned int MESSAGE_MAX_LENGTH = 1024;
const unsigned int REQUEST_TIMEOUT = 1000;
const unsigned int REPLAY_WAIT_TIMEOUT = 2;

struct addr_data
{
	std::string addr;
	unsigned int addr_length;
	unsigned int ServerNo;
	unsigned int requestID;
	vector<unsigned int> ServiceNo_vector;
};

typedef map<string, addr_data> addr_map;
typedef pair<string, addr_data> addr_pair;
typedef map<unsigned int, addr_pair> serverNo_map;
typedef map<unsigned int, map<string, addr_data>> serviceNo_map;
typedef pair<string, unsigned int> requestID_pair;
typedef map<requestID_pair, addr_pair> requestID_map;
typedef multimap<long, requestID_pair> timer_map;

class bus_router
{
	addr_map addr_map_;
	serverNo_map serverNo_map_;
	serviceNo_map serviceNo_map_;   
	requestID_map requestID_map_;   //管理应答dealer的地址和router的requestID
	timer_map timer_map_;
	
	void* router_socket_;
	char* p_addr_;
	char* p_msg_; 

	unsigned int addr_length_;
	unsigned int msg_length_;
	unsigned int requestID_;

public:
	bus_router();
	bus_router(void* socket);
	~bus_router();

	void run();

private:
	void router_loginReq(dzh_bus_interface::Bus_Head* bus);
	void router_loginRsp(dzh_bus_interface::Bus_Head* bus);
	void router_logoutReq(dzh_bus_interface::Bus_Head* bus);
	void router_logoutRsp(dzh_bus_interface::Bus_Head* bus);
	void router_other_dealer(dzh_bus_interface::Bus_Head* bus);

	void send_msg(void* addr, unsigned int addr_len, void* msg, unsigned int msg_len);
	void send_msg(void* addr, unsigned int addr_len, dzh_bus_interface::Bus_Head* bus);
	void send_msg(std::string& buf, int cmd, dzh_bus_interface::Bus_Head* bus);
	void send_error_msg_back(dzh_bus_interface::Bus_Head* bus, char* msg);
	
	int find_address_map(char* addr);

	void add_requestID_map(string addr, unsigned int requestID);
	void recycling_watcher();

	void print_map();
};

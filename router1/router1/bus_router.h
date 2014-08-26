#pragma once

#include <zmq.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <string.h>
#include <iostream>

#include "dzh_bus_interface.pb.h"
#include "zmq_helper.h"

using namespace std;
using namespace google;
using namespace protobuf;

const unsigned int ADDRESS_MAX_LENGTH = 64;
const unsigned int MESSAGE_MAX_LENGTH = 1024;

struct addr_data
{
	unsigned int addr_length;
	unsigned int ServerNo;
	unsigned int requestID;
	vector<unsigned int> ServiceNo_vector;
};

typedef map<string, addr_data> addr_map;
typedef pair<string, addr_data> addr_pair;
typedef map<unsigned int, addr_pair> serverNo_map;
typedef map<unsigned int, vector<addr_pair>> serviceNo_map;
typedef pair<string, unsigned int> requestID_pair;
typedef map<requestID_pair, addr_pair> requestID_map;

class bus_router
{
	addr_map addr_map_;
	addr_pair addr_pair_;
	serverNo_map serverNo_map_;
	serviceNo_map serviceNo_map_;
	requestID_map requestID_map_;   //索引是 应答dealer的地址和router的requestID

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
	void router_logoutRsp();
	void router_other_dealer(dzh_bus_interface::Bus_Head* bus);

	int send_msg(void* addr, unsigned int addr_len, void* msg, unsigned int msg_len);
	int send_msg_back(char* msg);
	void* Serialize(dzh_bus_interface::Bus_Head* bus, int& size);

	int find_address_map(char* addr);

	void add_requestID_map(string addr, unsigned int requestID);

	void print_map();
};

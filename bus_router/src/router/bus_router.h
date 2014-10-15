#pragma once

#include <zmq.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <string.h>
#include <iostream>

#include "yt/log/ostreamlogger.h"
#include "yt/log/log.h"
#include "yt/log/datefilelogger.h"
#include "serverconf.h"
#include "dzhbus.pb.h"

extern bool end_process;
extern ServerConf g_conf;

using namespace std;
using namespace google;
using namespace protobuf;
using namespace dzhyun;

const unsigned int ADDRESS_MAX_LENGTH = 64;
const unsigned int MESSAGE_MAX_LENGTH = 1024;
const unsigned int TIMEOUT = 1000;
const unsigned int REPLAY_WAIT_TIMEOUT = 5;

struct addr_data
{
	std::string addr;
	unsigned int addr_length;
	unsigned int ServerNo;
	long reset_time;
	long alive_time;
	uint64_t requestID;
	vector<string> ServiceName_vector;
};

struct request_string_data
{
	unsigned int command_id;
	vector<string> serviceName_vec;
	unsigned int serviceName_cnt;
};

typedef pair<string, addr_data> addr_pair;
typedef pair<string, uint64_t> requestID_pair;
typedef pair<long, uint64_t> timer_pair;
typedef pair<long, string> heartbeat_pair;

struct reset_timer_data
{
	requestID_pair req_requestID_pair;
	unsigned int times;
};

typedef map<string, addr_data> addr_map;
typedef map<heartbeat_pair, string> heartbeat_map;
typedef map<requestID_pair, addr_pair> requestID_map;
typedef map<string, addr_map> serviceName_map;
typedef map<timer_pair, reset_timer_data> timer_map;

class bus_router
{
	addr_map addr_map_;
	heartbeat_map heartbeat_map_;
	serviceName_map serviceName_map_;   
	requestID_map requestID_map_;   
	timer_map timer_map_;

	void* router_socket_;
	char* p_addr_;
	char* p_msg_; 

	unsigned int addr_length_;
	unsigned int msg_length_;
	unsigned int msg_local_len;
	uint64_t requestID_;

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
	int compare_addr(serviceName_map::iterator iter_servieName);
	int compare_len(uint64_t len_recv, uint64_t len_local);

	void add_requestID_map(string addr, uint64_t requestID);
	void recycling_watcher();
	void recycling_heartbeat();

	void split_command(string str, request_string_data& rd);
	void split_anyreq(string str, request_string_data& rd);	

	void clear();
};

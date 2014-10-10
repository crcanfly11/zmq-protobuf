#include "bus_router.h"

bus_router::bus_router() : router_socket_(NULL), addr_length_(0), msg_length_(0), requestID_(0), time_(0)
{
	p_addr_ = NULL; p_msg_ = NULL;
};

bus_router::bus_router(void* socket) : router_socket_(socket), addr_length_(0), msg_length_(0), requestID_(0), time_(0)
{
	p_addr_ = NULL; p_msg_ = NULL;
};

bus_router::~bus_router(void)
{
	ShutdownProtobufLibrary();

	clear();

	delete p_addr_; p_addr_ = NULL;
	delete p_msg_; p_msg_ = NULL;
};

void bus_router::run()
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	p_addr_ = new char[ADDRESS_MAX_LENGTH];
	p_msg_ = new char[MESSAGE_MAX_LENGTH*g_conf.m_msgmaxlen];

	while(1) {
		if(end_process)  
			break;

		zmq_pollitem_t items[] = { { router_socket_, 0, ZMQ_POLLIN, 0 } };
		int rc = zmq_poll(items, 1, TIMEOUT);
		if(rc == -1) {
			break;
		}
		if(items[0].revents & ZMQ_POLLIN) {
			addr_length_ = zmq_recv(router_socket_, p_addr_, ADDRESS_MAX_LENGTH, 0);
			msg_length_ = zmq_recv(router_socket_, p_msg_, MESSAGE_MAX_LENGTH, 0);

			p_addr_[addr_length_] = 0;
			p_msg_[msg_length_] = 0;

			BusHead bus;
			bus.ParseFromArray(p_msg_, msg_length_);

			request_string_data rd;
			split_command(bus.servicename(), rd);

			switch(rd.command_id)
			{
				case 1:
					router_loginReq(&bus, &rd);
					break;
				case 3:
					router_logoutReq(&bus);
					break;
				case 5:
					//router_ServiceRouteReqSToV(&bus);	
					break;
				case 7:
					//router_ServiceRouteReqVToS(&bus);
					break;
				case 8:
					//router_ServiceRouteStatusNty();
					break;
				case 9:
					//router_ServiceRouteReqVToV(&bus);
					break;
				case 11:
					reset_map(&bus);
					break;
				case 88:
					router_heartbeatReq(&bus);
					break;
				default:
					router_other_dealer(&bus);
					break;
			}
		}
		//retry connection
		else {
		}
		long tm = (long)time(NULL);
		if(tm > time_) {
			recycling_watcher();
			recycling_heartbeat();
			time_ = tm;
		}	
	}	
};
void bus_router::reset_map(BusHead* bus)
{
	clear();

	RspInfo rsp;
	rsp.set_rspno(0);
	char* msg("clear succeed");
	rsp.set_rspdesc(msg, strlen(msg));

	std::string rsp_str;
	rsp.SerializeToString(&rsp_str);

	BusHead bh;
	bh.set_servicename("reply/bus/rspinfo");
	bh.set_requestid(bus->requestid());
	bh.set_allocated_data(&rsp_str);
	
	int bh_size = bh.ByteSize();
	void* bh_buf = malloc(bh_size);
	bh.SerializeToArray(bh_buf, bh_size);	
	send_msg(p_addr_, addr_length_, bh_buf, bh_size);
	AC_INFO("requestID: %d, request clear map.", bus->requestid());

	free(bh_buf);	
	bh.release_data();
	bh.release_servicename();
	rsp.release_rspdesc();
};

void bus_router::split_command(std::string str, request_string_data& rd)
{
	if(strncmp((char*)str.substr(0,3).c_str(), "bus", 3) != 0) {
		rd.command_id = 0;
		return;
	}

	if(str.find("login") != string::npos)
		rd.command_id = 1;
	else if(str.find("logout") != string::npos)
		rd.command_id = 3;
	else if(str.find("clear") != string::npos)
		rd.command_id = 11;
	else if(str.find("ping") != string::npos)
		rd.command_id = 88;
	else 
		rd.command_id = 0;
};

void bus_router::split_anyreq(std::string str, request_string_data& rd)
{
	int idx_split = str.find("?");
	if(idx_split == (int)string::npos) { 
		rd.serviceName_cnt = 0;
		return;
	}

	rd.serviceName_vec.clear();

	int idx_comma_begin = str.find("=", idx_split);
	int idx_comma_end = str.find(",", idx_comma_begin+1);

	if(idx_comma_end == (int)string::npos) {
		string str_only = &str[idx_comma_begin+1];
		if(!str_only.empty())
			rd.serviceName_vec.push_back(str_only);
	}
	else {
		if(idx_comma_begin != (int)string::npos) {
			rd.serviceName_vec.push_back(str.substr(idx_comma_begin+1, idx_comma_end-idx_comma_begin-1));	
			idx_comma_begin = idx_comma_end;
			while(1) {
				idx_comma_end = str.find(",", idx_comma_begin+1);
				if(idx_comma_begin != (int)string::npos) {
					rd.serviceName_vec.push_back(str.substr(idx_comma_begin+1, idx_comma_end-idx_comma_begin-1));	
					idx_comma_begin = idx_comma_end;
				}	
				else 
					break;
			}
		}
	}
	
	rd.serviceName_cnt = rd.serviceName_vec.size();	

	return;
};

void bus_router::router_loginReq(BusHead* bus, request_string_data* rd)
{
	AnyReq loginReq;
	loginReq.ParseFromString(bus->data());

	if(!find_address_map(p_addr_)) {
		//send_error_msg_back(bus, "already logined.");
		//return;
		router_loginRsp(bus, rd);
	}

	split_anyreq(loginReq.url(), *rd);

	addr_data adt;
	adt.addr.assign(p_addr_, addr_length_);
	adt.addr_length = addr_length_;
	adt.ServerNo = 0;
	adt.alive_time = (long)time(NULL) + g_conf.m_heartbeatouttime + 2; 
	//adt.ServerNo = loginReq.serverno();

	unsigned int size = rd->serviceName_cnt; 
	for(unsigned int i=0; i<size; ++i) {
		adt.ServiceName_vector.push_back(rd->serviceName_vec[i]);
	}

	//add address
	addr_map_.insert(addr_pair(string(p_addr_, addr_length_), adt));

	//set heartbeat
	heartbeat_pair hp(adt.alive_time, string(p_addr_, addr_length_));
	heartbeat_map_.insert(pair<heartbeat_pair, string>(hp, string(p_addr_, addr_length_)));

	//add service No
	if(!adt.ServiceName_vector.empty()) {
		for(unsigned int i=0; i<size; ++i) {
			serviceName_map::iterator iter_serviceName = serviceName_map_.find(adt.ServiceName_vector[i]);
			if(iter_serviceName != serviceName_map_.end()) {
				iter_serviceName->second.insert(pair<string, addr_data>(string(p_addr_, addr_length_), adt));
			}
			else {
				map<string, addr_data> string_map;
				string_map.insert(addr_pair(string(p_addr_, addr_length_), adt));
				serviceName_map_.insert(pair<string, map<string, addr_data> >(adt.ServiceName_vector[i], string_map));
			}
		}	
	}

	AC_INFO("request ID:%d login.", bus->requestid());

	//应答
	router_loginRsp(bus, rd);
};

void bus_router::router_loginRsp(BusHead* bus, request_string_data* rd)
{
	int size = rd->serviceName_cnt; 

	char buf_serviceno[64];
	sprintf(buf_serviceno, "register %d service. ", size);
	
	char buf_str[128] = "login succeed. ";

	if(size != 0) 
		strncat(buf_str, buf_serviceno, sizeof(buf_serviceno));
	
	RspInfo rsp;
	rsp.set_rspno(0);
	std::string str = buf_str;
	rsp.set_allocated_rspdesc(&str);

	LoginRsp lgin_rsp;
	lgin_rsp.set_allocated_rspinfo(&rsp);

	string lgin_rsp_str;
	lgin_rsp.SerializeToString(&lgin_rsp_str);

	BusHead bh;
	bh.set_servicename("reply/bus/login");
	bh.set_requestid(bus->requestid());
	bh.set_allocated_data(&lgin_rsp_str);
	
	int bh_size = bh.ByteSize();
	void* bh_buf = malloc(bh_size);
	bh.SerializeToArray(bh_buf, bh_size);	
	send_msg(p_addr_, addr_length_, bh_buf, bh_size);

	free(bh_buf);	
	bh.release_data();
	bh.release_servicename();
	rsp.release_rspdesc();
	lgin_rsp.release_rspinfo();
};

void bus_router::router_logoutReq(BusHead* bus)
{
	addr_map::iterator iter_addr_map = addr_map_.find(string(p_addr_, addr_length_));
	if(iter_addr_map == addr_map_.end()) {
		send_error_msg_back(bus, "unregistered.");
		return;
	}

	vector<string> lgout_serviceName_vec = iter_addr_map->second.ServiceName_vector;
	int size = lgout_serviceName_vec.size();
	for(int i=0;i<size;++i) {
		serviceName_map::iterator iter_serviceName_map = serviceName_map_.find(lgout_serviceName_vec[i]);
		if(iter_serviceName_map == serviceName_map_.end()) {
			continue;
		}
		else {
			if(iter_serviceName_map->second.size() == 1) {
				serviceName_map_.erase(iter_serviceName_map);
			}
			else {
				map<string, addr_data>::iterator iter_string = iter_serviceName_map->second.find(string(p_addr_, addr_length_));
				if(iter_string == iter_serviceName_map->second.end()) {
					continue;				
				}
				else {
					iter_serviceName_map->second.erase(iter_string);
				}
			}		
		}
	}

	heartbeat_pair hp(iter_addr_map->second.alive_time, string(p_addr_, addr_length_));
	heartbeat_map::iterator iter_heartbeat = heartbeat_map_.find(hp);
	if(iter_heartbeat != heartbeat_map_.end()) {
		heartbeat_map_.erase(iter_heartbeat);
	}

	addr_map_.erase(iter_addr_map);	

	AC_INFO("request ID:%d logout.", bus->requestid());

	router_logoutRsp(bus);
};

void bus_router::router_logoutRsp(BusHead* bus)
{
	RspInfo rsp;
	rsp.set_rspno(0);
	std::string str("logout succeed.");
	rsp.set_allocated_rspdesc(&str);

	LogoutRsp lgout_rsp;
	lgout_rsp.set_allocated_rspinfo(&rsp);

	std::string lgout_rsp_str;
	lgout_rsp.SerializeToString(&lgout_rsp_str);

	BusHead bh;
	bh.set_servicename("reply/bus/logout");
	bh.set_requestid(bus->requestid());
	bh.set_allocated_data(&lgout_rsp_str);
	
	int bh_size = bh.ByteSize();
	void* bh_buf = malloc(bh_size);
	bh.SerializeToArray(bh_buf, bh_size);	
	send_msg(p_addr_, addr_length_, bh_buf, bh_size);

	free(bh_buf);	
	bh.release_data();
	bh.release_servicename();
	rsp.release_rspdesc();
	lgout_rsp.release_rspinfo();
};

void bus_router::router_heartbeatReq(BusHead* bus)
{
	addr_map::iterator iter_addr_map = addr_map_.find(string(p_addr_, addr_length_));

	//unregistered addr do nothing
	if(iter_addr_map == addr_map_.end())
		return;  

	addr_data ad = iter_addr_map->second; 
	heartbeat_map::iterator iter_heartbeat = heartbeat_map_.find(heartbeat_pair(ad.alive_time, string(p_addr_, addr_length_)));
	if(iter_heartbeat == heartbeat_map_.end()) {
		//doing something
		return;
	}

	long tm = (long)time(NULL) + g_conf.m_heartbeatouttime + 2;

	heartbeat_map_.erase(iter_heartbeat);
	heartbeat_pair hp(tm, string(p_addr_, addr_length_));
	heartbeat_map_.insert(pair<heartbeat_pair, string>(hp, string(p_addr_, addr_length_)));

	iter_addr_map->second.alive_time = tm;

	//heartbeat rsp
	router_heartbeatRsp(bus);
};

void bus_router::router_heartbeatRsp(BusHead* bus)
{
	//send rsp
	RspInfo rsp;
	rsp.set_rspno(0);
	std::string str = "heartbeat response.";
	rsp.set_allocated_rspdesc(&str);

	string heartbeat_rsp_str;
	rsp.SerializeToString(&heartbeat_rsp_str);

	BusHead bh;
	bh.set_servicename("reply/bus/ping");
	bh.set_requestid(bus->requestid());
	bh.set_allocated_data(&heartbeat_rsp_str);
	
	int bh_size = bh.ByteSize();
	void* bh_buf = malloc(bh_size);
	bh.SerializeToArray(bh_buf, bh_size);	
	send_msg(p_addr_, addr_length_, bh_buf, bh_size);

	free(bh_buf);	
	bh.release_data();
	bh.release_servicename();
	rsp.release_rspdesc();
};

int bus_router::compare_addr(serviceName_map::iterator iter_servieName)
{
	map<string, addr_data>::iterator iter_string = iter_servieName->second.begin();
	for(;iter_string != iter_servieName->second.end();++iter_string) {
		if(memcmp(p_addr_, iter_string->second.addr.c_str(), iter_string->second.addr_length) == 0)
			return -1;
	}
	return 0;
};

void bus_router::router_other_dealer(BusHead* bus)
{
	requestID_map::iterator iter_requestID = requestID_map_.find(requestID_pair(string(p_addr_, addr_length_), bus->requestid()));
	if(iter_requestID == requestID_map_.end()) {
		serviceName_map::iterator iter_servieName = serviceName_map_.find(bus->servicename());
		if(iter_servieName == serviceName_map_.end()) {
			send_error_msg_back(bus, "unregistered body type no.");
			return;
		}
		else {    
			if(compare_addr(iter_servieName) != 0) {
				send_error_msg_back(bus, "not request self service.");
				return;
			}
			
			int size = iter_servieName->second.size();
			map<string, addr_data>::iterator iter_string = iter_servieName->second.begin();
			
			//随机地址
			srand((unsigned int)time(NULL));
			for(int i = rand()%size; i!=0; --i) 
				++iter_string;
			
			addr_data ad = iter_string->second;
			add_requestID_map(ad.addr, bus->requestid());

			bus->set_requestid(requestID_);
			
			send_msg((void*)ad.addr.c_str(), ad.addr_length, bus);

			AC_INFO("request ID:%d forwarding. bag size:%d", bus->requestid(), ad.addr_length);
		}			
	}
	else {
		addr_pair rep_addr = iter_requestID->second;
		bus->set_requestid(rep_addr.second.requestID);

		send_msg((void*)rep_addr.first.c_str(), rep_addr.second.addr_length, bus);

		if(bus->endflag() == 0) {
			requestID_map_.erase(iter_requestID);
			AC_INFO("request ID:%d reply.", bus->requestid());
		}
		else {
			//reset timer 
			timer_map::iterator iter_timer = timer_map_.find(timer_pair(iter_requestID->second.second.reset_time, iter_requestID->first.second));
			if(iter_timer != timer_map_.end()) {
				iter_timer->second.times = iter_timer->second.times + 1;
				AC_INFO("request ID:%d reset timer.", bus->requestid());	
			}
		}
	}
};

void bus_router::add_requestID_map(string addr, uint64_t requestID)
{
	++requestID_;
	long tm = (long)time(NULL)+g_conf.m_replyouttime;

	requestID_pair req_requestID(addr, requestID_);

	addr_data ad;
	ad.addr_length = addr_length_;
	ad.requestID = requestID;
	ad.reset_time = tm;
	addr_pair req_addr(string(p_addr_, addr_length_), ad);

	requestID_map_.insert(pair<requestID_pair, addr_pair>(req_requestID, req_addr));
	
	//add timer map 
	reset_timer_data rtd;
	rtd.req_requestID_pair = req_requestID;
	rtd.times = 0;

	timer_pair pf(tm, requestID_);
	timer_map_.insert(pair<timer_pair, reset_timer_data>(pf, rtd));
};

void bus_router::send_msg(void* addr, unsigned int addr_len, void* msg, unsigned int msg_len)
{
	zmq_send(router_socket_, addr, addr_len, ZMQ_SNDMORE);
	zmq_send(router_socket_, msg, msg_len, 0);
};

void bus_router::send_msg(void* addr, unsigned int addr_len, BusHead* bus)
{
	int size_buf = bus->ByteSize();
	void* buf = malloc(size_buf);
	bus->SerializeToArray(buf, size_buf);

	send_msg(addr, addr_len, buf, size_buf);

	free(buf);	
}

void bus_router::send_error_msg_back(BusHead* bus, char* msg)
{
	RspInfo rsp;
	rsp.set_rspno(-1);                  //错误编号 没有对应关系
	rsp.set_rspdesc(msg, strlen(msg));

	std::string rsp_str;
	rsp.SerializeToString(&rsp_str);

	BusHead bh;
	bh.set_servicename("reply/bus/rspinfo");
	bh.set_requestid(bus->requestid());
	bh.set_allocated_data(&rsp_str);
	
	int bh_size = bh.ByteSize();
	void* bh_buf = malloc(bh_size);
	bh.SerializeToArray(bh_buf, bh_size);	
	send_msg(p_addr_, addr_length_, bh_buf, bh_size);

	free(bh_buf);	
	bh.release_data();
	bh.release_servicename();
	rsp.release_rspdesc();
};

int bus_router::find_address_map(char* addr)
{
	addr_map::iterator iter_addr_map = addr_map_.find(addr);
	if(iter_addr_map == addr_map_.end())
		return -1;

	return 0;
};

void bus_router::recycling_watcher()
{
	if(timer_map_.empty()) return;
	
	long tm = (long)time(NULL);

	timer_map::iterator iter_timer = timer_map_.begin();
	for(; iter_timer != timer_map_.end();) {
		if( tm < iter_timer->first.first + g_conf.m_replyouttime*iter_timer->second.times ) 
			return;

		//send error msg 
		requestID_map::iterator iter_request = requestID_map_.find(iter_timer->second.req_requestID_pair);
		if(iter_request == requestID_map_.end()) {
			timer_map::iterator iter_timer_erase = iter_timer++;
			timer_map_.erase(iter_timer_erase);
			continue;
		}
		
		router_timeoutRsp(iter_request->second);		

		timer_map::iterator iter_timer_erase = iter_timer++;
		timer_map_.erase(iter_timer_erase);
		
		requestID_map_.erase(iter_request);
	} 
};

void bus_router::router_timeoutRsp(addr_pair& pair)
{
	BusHead rep_bh;
	rep_bh.set_servicename("reply/bus/rspinfo");
	rep_bh.set_requestid(pair.second.requestID);

	std::string str;
	addr_map::iterator iter_addr = addr_map_.find(pair.first);
	if(iter_addr == addr_map_.end()) {
		str = "lost connect with app";
	}
	else 
		str = "replay time out.";

	RspInfo rsp;
	rsp.set_rspno(-1);
	rsp.set_allocated_rspdesc(&str);

	std::string rsp_str;
	rsp.SerializeToString(&rsp_str);

	rep_bh.set_allocated_data(&rsp_str);

	int bh_size = rep_bh.ByteSize();
	void* bh_buf = malloc(bh_size);
	rep_bh.SerializeToArray(bh_buf, bh_size);	

	send_msg((void*)pair.first.c_str(), pair.second.addr_length, bh_buf, bh_size);		
	AC_INFO("request ID:%d reply time out.", pair.second.requestID); 

	free(bh_buf);	
	rsp.release_rspdesc();
	rep_bh.release_data();
	rep_bh.release_servicename();
};

void bus_router::recycling_heartbeat()
{
	long tm = (long)time(NULL);
	heartbeat_map::iterator iter_heartbeat = heartbeat_map_.begin();
	for(;iter_heartbeat != heartbeat_map_.end();) {
		if(tm < iter_heartbeat->first.first)
			return;
		else {
			heartbeat_map::iterator iter = iter_heartbeat++;
			addr_map::iterator iter_addr = addr_map_.find(iter->second);
			if(iter_addr != addr_map_.end()) {
				int service_size = iter_addr->second.ServiceName_vector.size();
				for(int i=0;i < service_size;++i) {
					serviceName_map::iterator iter_service_name = serviceName_map_.find(iter_addr->second.ServiceName_vector[i]);
					if(iter_service_name != serviceName_map_.end()) {
						if(iter_service_name->second.size() == 1) {
							serviceName_map_.erase(iter_service_name);
						}
						else {
							addr_map::iterator iter_erase = iter_service_name->second.find(iter->second);
							if(iter_erase != iter_service_name->second.end()) {
								iter_service_name->second.erase(iter_erase);
							}
						}
					}
				}
			}

			addr_map_.erase(iter->second);
			heartbeat_map_.erase(iter);

			//cout<< "erase add size: "<< addr_map_.size()<< endl;
			//cout<< "erase heartbeat size: "<< heartbeat_map_.size()<< endl;
		}
	}
};

void bus_router::clear()
{
	addr_map::iterator iter_addr_map = addr_map_.begin();
	for(; iter_addr_map != addr_map_.end(); ++iter_addr_map) {
		iter_addr_map->second.addr.clear();
		if(!iter_addr_map->second.ServiceName_vector.empty()) {
			iter_addr_map->second.ServiceName_vector.clear();
		}
	}
	addr_map_.clear();
	heartbeat_map_.clear();

	serviceName_map::iterator iter_serviceName_map = serviceName_map_.begin();
	for(; iter_serviceName_map != serviceName_map_.end(); ++iter_serviceName_map) {
		map<string, addr_data>::iterator iter_map = iter_serviceName_map->second.begin();
		for(; iter_map != iter_serviceName_map->second.end(); ++iter_map) {
			if(!iter_map->second.ServiceName_vector.empty()) 
				iter_map->second.ServiceName_vector.clear();
		}
		iter_serviceName_map->second.clear();
	}
	serviceName_map_.clear();

	requestID_map::iterator iter_requestID_map = requestID_map_.begin();
	for(; iter_requestID_map != requestID_map_.end(); ++iter_requestID_map) {
		if(!iter_requestID_map->second.second.ServiceName_vector.empty())
			iter_requestID_map->second.second.ServiceName_vector.clear();
	} 
	requestID_map_.clear();
	timer_map_.clear();
};

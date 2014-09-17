#include "bus_router.h"

bus_router::bus_router() : router_socket_(NULL), addr_length_(0), msg_length_(0), requestID_(0)
{
	p_addr_ = NULL; p_msg_ = NULL;
};

bus_router::bus_router(void* socket) : router_socket_(socket), addr_length_(0), msg_length_(0), requestID_(0)
{
	p_addr_ = NULL; p_msg_ = NULL;
};

bus_router::~bus_router(void)
{
	ShutdownProtobufLibrary();
};

void bus_router::run()
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	p_addr_ = new char[ADDRESS_MAX_LENGTH];
	p_msg_ = new char[MESSAGE_MAX_LENGTH];

	while(1) {
		if(end_process)  
			break;

		zmq_pollitem_t items [] = { { router_socket_, 0, ZMQ_POLLIN, 0 } };  
		int rc = zmq_poll (items, 1, REQUEST_TIMEOUT);  
		if (rc == -1) {
			break; // Interrupted  
		}
        if (items [0].revents & ZMQ_POLLIN) {  
            // 接收反馈  
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
				default:
					router_other_dealer(&bus);
					break;
			}
        }  
        // 重试连接  
        else {  
            //printf("Retry connecting ...\n");  
            //zmq_close (zsocket);  
            //zsocket = zmq_socket_new(context);  
            //// 重发消息  
            //zmq_send (zsocket, send_s, strlen(send_s), 0);  
            //printf ("Send %s\n", send_s);  
            //--retries_left;  
        } 
		recycling_watcher();
	}	
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
	else 
		rd.command_id = 0;
};

void bus_router::split_anyreq(std::string str, request_string_data& rd)
{
	int idx_split = str.find("?");
	if(idx_split == string::npos) { 
		rd.serviceName_cnt = 0;
		return;
	}

	rd.serviceName_vec.clear();

	int idx_comma_begin = str.find("=", idx_split);
	int idx_comma_end = str.find(",", idx_comma_begin);

	if(idx_comma_end == string::npos) {
		string str_only = &str[idx_comma_begin+1];
		if(!str_only.empty())
			rd.serviceName_vec.push_back(str_only);	
	}
	else {
		if(idx_comma_begin != string::npos) {
			rd.serviceName_vec.push_back(str.substr(idx_comma_begin+1, idx_comma_end-idx_comma_begin-1));	
			idx_comma_begin = idx_comma_end;
			while(1) {
				idx_comma_end = str.find(",", idx_comma_begin+1);
				if(idx_comma_begin != string::npos) {
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

void bus_router::print_map()
{
	cout<< "---------addr_map_---------"<< endl;

	addr_map::iterator iter_print = addr_map_.begin();
	for(;iter_print!=addr_map_.end();++iter_print) {
		cout<< iter_print->first<< endl;
	}

	cout<< "---------serviceNo_map_---------"<< endl;

	service_Name_map::iterator iter_service_print = service_Name_map_.begin();
	for(;iter_service_print!= service_Name_map_.end();++iter_service_print) {
		cout<< iter_service_print->first<< ", ";
		map<string, addr_data> ve = iter_service_print->second;
		map<string, addr_data>::iterator iter_map = ve.begin();
		for(;iter_map!=ve.end();++iter_map) {
			cout<< iter_map->first<< ", "<< iter_map->second.addr_length;
		}

		cout<<endl;
	}
	cout<< "---------end---------"<< endl;	
};

void bus_router::router_loginReq(BusHead* bus, request_string_data* rd)
{
	AnyReq loginReq;
	loginReq.ParseFromString(bus->data());

	if(!find_address_map(p_addr_)) {
		send_error_msg_back(bus, "already logined.");
		return;
	}
	
	split_anyreq(loginReq.url(), *rd);
	addr_data adt;
	adt.addr.assign(p_addr_, addr_length_);
	adt.addr_length = addr_length_;
	adt.ServerNo = 0;
	//adt.ServerNo = loginReq.serverno();
	
	unsigned int size = rd->serviceName_cnt;
	for(unsigned int i=0; i<size; ++i) {
		adt.ServiceName_vector.push_back(rd->serviceName_vec[i]);
	}

	//add address
	addr_map_.insert(addr_pair(string(p_addr_, addr_length_),adt));

	//业务逻辑未确定
	//serverNo_map_.insert(pair<unsigned int, addr_pair>(adt.ServerNo, addr_pair(p_addr_,adt)));

	//add service No
	if(!adt.ServiceName_vector.empty()) {
		for(unsigned int i=0; i<size; ++i) {
			service_Name_map::iterator iter_serviceName = service_Name_map_.find(adt.ServiceName_vector[i]);
			if(iter_serviceName != service_Name_map_.end()) {
				iter_serviceName->second.insert(pair<string, addr_data>(string(p_addr_, addr_length_), adt));
			}
			else {
				map<string, addr_data> string_map;
				string_map.insert(addr_pair(string(p_addr_, addr_length_), adt));
				service_Name_map_.insert(pair<string, map<string, addr_data>>(adt.ServiceName_vector[i], string_map));
			}
		}	
	}

	//print_map();
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
		service_Name_map::iterator iter_serviceName_map = service_Name_map_.find(lgout_serviceName_vec[i]);
		if(iter_serviceName_map == service_Name_map_.end()) {
			continue;
		}
		else {
			if(iter_serviceName_map->second.size() == 1) {
				service_Name_map_.erase(iter_serviceName_map);
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

void bus_router::router_other_dealer(BusHead* bus)
{
	requestID_map::iterator iter_requestID = requestID_map_.find(requestID_pair(string(p_addr_, addr_length_), bus->requestid()));
	if(iter_requestID == requestID_map_.end()) {
		service_Name_map::iterator iter_servieName = service_Name_map_.find(bus->servicename());
		if(iter_servieName == service_Name_map_.end()) {
			send_error_msg_back(bus, "unregistered body type no.");
			return;
		}
		else {    
			int size = iter_servieName->second.size();
			map<string, addr_data>::iterator iter_string = iter_servieName->second.begin();
			
			//随机地址
			srand(0);
			for(int i = rand()%size; i!=0; --i) 
				++iter_string;
			
			addr_data ad = iter_string->second;
			add_requestID_map(ad.addr, bus->requestid());

			bus->set_requestid(requestID_);
			
			send_msg((void*)ad.addr.c_str(), ad.addr_length, bus);
		}			
	}
	else {
		addr_pair rep_addr = iter_requestID->second;
		bus->set_requestid(rep_addr.second.requestID);

		send_msg((void*)rep_addr.first.c_str(), rep_addr.second.addr_length, bus);

		requestID_map_.erase(iter_requestID);
	}
};

void bus_router::add_requestID_map(string addr, unsigned int requestID)
{
	++requestID_;

	requestID_pair req_requestID(addr, requestID_);

	addr_data ad;
	ad.addr_length = addr_length_;
	ad.requestID = requestID;
	addr_pair req_addr(string(p_addr_, addr_length_), ad);

	requestID_map_.insert(pair<requestID_pair, addr_pair>(req_requestID, req_addr));

	//cout<< "--------requestID pair----------"<<endl;
	//map<requestID_pair, addr_pair>::iterator iter_map = requestID_map_.begin();
	//for(;iter_map != requestID_map_.end();++iter_map) {
	//	requestID_pair p_req = iter_map->first;
	//	cout<< p_req.first<< ", "<<p_req.second<< ",";

	//	addr_pair p_addr = iter_map->second;
	//	cout<< p_addr.first<< ", "<< endl;
	//}

    //add timer map
    timer_map_.insert(pair<long, requestID_pair>((long)time(NULL)+REPLAY_WAIT_TIMEOUT, req_requestID));
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
	cout<< "--------------address map size--------------"<< endl;
	cout<< addr_map_.size()<< endl;
	cout<< "--------------addr map content--------------"<< endl;
	addr_map::iterator iter_print = addr_map_.begin();
	for(;iter_print!=addr_map_.end();++iter_print) {
		cout<< iter_print->first<< endl;
	}	

	cout<< "--------------find address--------------"<< endl;
	cout<< addr<< endl;

	addr_map::iterator iter_addr_map = addr_map_.find(addr);
	if(iter_addr_map == addr_map_.end())
		return -1;

	return 0;
};

void bus_router::recycling_watcher()
{
	//send_msg("hi", 2, "rsp hi", 6);
	//send_msg("hello", 5, "rsp hello", 9);

	if(timer_map_.empty()) return;

	long tm = (long)time(NULL);
	timer_map::iterator iter_timer = timer_map_.begin();
	for(; iter_timer != timer_map_.end();) {
		if( tm < iter_timer->first ) {
			return;
		}

		//send error msg 
		requestID_map::iterator iter_request = requestID_map_.find(iter_timer->second);
		if(iter_request == requestID_map_.end()) {
			timer_map::iterator iter_timer_erase = iter_timer++;
			timer_map_.erase(iter_timer_erase);
			continue;
		}

		addr_pair rep_addr = iter_request->second;
		
		BusHead rep_bh;
		rep_bh.set_servicename("reply/bus/rspinfo");
		rep_bh.set_requestid(rep_addr.second.requestID);

		std::string str("replay time out.");
		RspInfo rsp;
		rsp.set_rspno(-1);
		rsp.set_allocated_rspdesc(&str);

		std::string rsp_str;
		rsp.SerializeToString(&rsp_str);

		rep_bh.set_allocated_data(&rsp_str);

		int bh_size = rep_bh.ByteSize();
		void* bh_buf = malloc(bh_size);
		rep_bh.SerializeToArray(bh_buf, bh_size);	
	
		send_msg((void*)rep_addr.first.c_str(), rep_addr.second.addr_length, bh_buf, bh_size);		
	
		free(bh_buf);	
		rsp.release_rspdesc();
		rep_bh.release_data();
		rep_bh.release_servicename();

		timer_map::iterator iter_timer_erase = iter_timer++;
		timer_map_.erase(iter_timer_erase);

		requestID_map_.erase(iter_request);
	}
};
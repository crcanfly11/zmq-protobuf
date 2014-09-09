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

			dzh_bus_interface::Bus_Head bus;
			bus.ParseFromArray(p_msg_, msg_length_);

			switch(bus.command())
			{
				case 1:
					router_loginReq(&bus);
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

void bus_router::print_map()
{
	cout<< "---------addr_map_---------"<< endl;

	addr_map::iterator iter_print = addr_map_.begin();
	for(;iter_print!=addr_map_.end();++iter_print) {
		cout<< iter_print->first<< endl;
	}

	cout<< "---------serviceNo_map_---------"<< endl;

	serviceNo_map::iterator iter_service_print = serviceNo_map_.begin();
	for(;iter_service_print!= serviceNo_map_.end();++iter_service_print) {
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

void bus_router::router_loginReq(dzh_bus_interface::Bus_Head* bus)
{
	dzh_bus_interface::LoginReq loginReq;
	loginReq.ParseFromString(bus->data());

	if(!find_address_map(p_addr_)) {
		send_error_msg_back(bus, "already logined.");
		return;
	}

	addr_data adt;
	adt.addr = p_addr_;
	adt.addr_length = addr_length_;
	adt.ServerNo = 0;
	//adt.ServerNo = loginReq.serverno();

	int size = loginReq.serviceno_size();
	for(int i=0; i<size; ++i) {
		adt.ServiceNo_vector.push_back(loginReq.serviceno(i));
	}

	//add address
	addr_map_.insert(addr_pair(p_addr_,adt));

	//业务逻辑未确定
	//serverNo_map_.insert(pair<unsigned int, addr_pair>(adt.ServerNo, addr_pair(p_addr_,adt)));

	//add service No
	if(!adt.ServiceNo_vector.empty()) {
		for(int i=0; i<size; ++i) {
			serviceNo_map::iterator iter_serviceNo = serviceNo_map_.find(adt.ServiceNo_vector[i]);
			if(iter_serviceNo != serviceNo_map_.end()) {
				iter_serviceNo->second.insert(pair<string, addr_data>(p_addr_, adt));
			}
			else {
				map<string, addr_data> string_map;
				string_map.insert(addr_pair(p_addr_, adt));
				serviceNo_map_.insert(pair<unsigned int, map<string, addr_data>>(adt.ServiceNo_vector[i], string_map));
			}
		}	
	}

	//print_map();

	//应答
	router_loginRsp(bus);
};

void bus_router::router_loginRsp(dzh_bus_interface::Bus_Head* bus)
{
	dzh_bus_interface::LoginReq loginReq;
	loginReq.ParseFromString(bus->data());

	int size = loginReq.serviceno_size();

	char buf_serviceno[64];
	sprintf(buf_serviceno, "register %d service. ", size);
	
	char buf_str[128] = "login succeed. ";

	if(size != 0) 
		strncat(buf_str, buf_serviceno, sizeof(buf_serviceno));

	dzh_bus_interface::RspInfo rsp;
	rsp.set_rspno(0);
	std::string str = buf_str;
	rsp.set_allocated_rspdesc(&str);

	dzh_bus_interface::LoginRsp lgin_rsp;
	lgin_rsp.set_allocated_rspinfo(&rsp);

	string lgin_rsp_str;
	lgin_rsp.SerializeToString(&lgin_rsp_str);

	send_msg(lgin_rsp_str, 2, bus);

	rsp.release_rspdesc();
	lgin_rsp.release_rspinfo();
};

void bus_router::router_logoutReq(dzh_bus_interface::Bus_Head* bus)
{
	addr_map::iterator iter_addr_map = addr_map_.find(p_addr_);
	if(iter_addr_map == addr_map_.end()) {
		send_error_msg_back(bus, "unregistered.");
		return;
	}

	vector<unsigned int> lgout_serviceNo_vec = iter_addr_map->second.ServiceNo_vector;
	int size = lgout_serviceNo_vec.size();
	for(int i=0;i<size;++i) {
		serviceNo_map::iterator iter_serviceNo_map = serviceNo_map_.find(lgout_serviceNo_vec[i]);
		if(iter_serviceNo_map == serviceNo_map_.end()) {
			continue;
		}
		else {
			if(iter_serviceNo_map->second.size() == 1) {
				serviceNo_map_.erase(iter_serviceNo_map);
			}
			else {
				map<string, addr_data>::iterator iter_string = iter_serviceNo_map->second.find(p_addr_);
				if(iter_string == iter_serviceNo_map->second.end()) {
					continue;				
				}
				else {
					iter_serviceNo_map->second.erase(iter_string);
				}
			}		
		}
	}

	router_logoutRsp(bus);
};

void bus_router::router_logoutRsp(dzh_bus_interface::Bus_Head* bus)
{
	dzh_bus_interface::RspInfo rsp;
	rsp.set_rspno(0);
	std::string str("logout succeed.");
	rsp.set_allocated_rspdesc(&str);

	dzh_bus_interface::LogoutRsp lgout_rsp;
	lgout_rsp.set_allocated_rspinfo(&rsp);

	std::string lgout_rsp_str;
	lgout_rsp.SerializeToString(&lgout_rsp_str);

	send_msg(lgout_rsp_str, 4, bus);

	rsp.release_rspdesc();
	lgout_rsp.release_rspinfo();
};

void bus_router::router_other_dealer(dzh_bus_interface::Bus_Head* bus)
{
	requestID_map::iterator iter_requestID = requestID_map_.find(requestID_pair(p_addr_, bus->requestid()));
	if(iter_requestID == requestID_map_.end()) {
		serviceNo_map::iterator iter_servieNo = serviceNo_map_.find(bus->serviceno());
		if(iter_servieNo == serviceNo_map_.end()) {
			send_error_msg_back(bus, "unregistered body type no.");
			return;
		}
		else {    
			int size = iter_servieNo->second.size();
			map<string, addr_data>::iterator iter_string = iter_servieNo->second.begin();
			
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
	addr_pair req_addr(p_addr_, ad);

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

void bus_router::send_msg(void* addr, unsigned int addr_len, dzh_bus_interface::Bus_Head* bus)
{
	int size_buf = bus->ByteSize();
	void* buf = malloc(size_buf);
	bus->SerializeToArray(buf, size_buf);

	send_msg(addr, addr_len, buf, size_buf);

	free(buf);	
}

void bus_router::send_msg(std::string& buf, int cmd, dzh_bus_interface::Bus_Head* bus)
{
	dzh_bus_interface::Bus_Head bh;
	bh.set_command(cmd);
	bh.set_requestid(bus->requestid());
	bh.set_allocated_data(&buf);
	
	int bh_size = bh.ByteSize();
	void* bh_buf = malloc(bh_size);
	bh.SerializeToArray(bh_buf, bh_size);	
	send_msg(p_addr_, addr_length_, bh_buf, bh_size);
	
	free(bh_buf);	
	bh.release_data();
}

void bus_router::send_error_msg_back(dzh_bus_interface::Bus_Head* bus, char* msg)
{
	dzh_bus_interface::RspInfo rsp;
	rsp.set_rspno(0);
	rsp.set_rspdesc(msg, strlen(msg));

	std::string rsp_str;
	rsp.SerializeToString(&rsp_str);

	send_msg(rsp_str, 0 ,bus);

	rsp.release_rspdesc();
};

int bus_router::find_address_map(char* addr)
{
	//cout<< "--------------address map size--------------"<< endl;
	//cout<< addr_map_.size()<< endl;
	//cout<< "--------------addr map content--------------"<< endl;
	//addr_map::iterator iter_print = addr_map_.begin();
	//for(;iter_print!=addr_map_.end();++iter_print) {
	//	cout<< iter_print->first<< endl;
	//}	

	//cout<< "--------------find address--------------"<< endl;
	//cout<< addr<< endl;

	addr_map::iterator iter_addr_map = addr_map_.find(addr);
	if(iter_addr_map == addr_map_.end())
		return -1;

	return 0;
};

void bus_router::recycling_watcher()
{
	if(timer_map_.empty() == true) return;

	long tm = (long)time(NULL);
	timer_map::iterator iter_timer = timer_map_.begin();
	for(; iter_timer != timer_map_.end();) {
		if( tm < iter_timer->first ) {
			return;
		}

		//send error msg 
		requestID_map::iterator iter_request = requestID_map_.find(iter_timer->second);
		addr_pair rep_addr = iter_request->second;
		
		dzh_bus_interface::Bus_Head rep_bh;
		rep_bh.set_requestid(rep_addr.second.requestID);

		std::string str("replay time out.");
		dzh_bus_interface::RspInfo rsp;
		rsp.set_rspno(0);
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

		timer_map::iterator iter_timer_erase = iter_timer++;
		timer_map_.erase(iter_timer_erase);
	}
};
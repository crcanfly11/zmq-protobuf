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
		memset(p_addr_, 0x00, ADDRESS_MAX_LENGTH);
		memset(p_msg_, 0x00, MESSAGE_MAX_LENGTH);

		addr_length_ = zmq_recv(router_socket_, p_addr_, ADDRESS_MAX_LENGTH, 0);
		msg_length_ = zmq_recv(router_socket_, p_msg_, MESSAGE_MAX_LENGTH, 0);

		dzh_bus_interface::Bus_Head bus;
		bus.ParseFromArray(p_msg_, msg_length_);

		switch(bus.bodytype())
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
		cout<< iter_service_print->first;
		vector<addr_pair> ve = iter_service_print->second;
		for(int i=0;i < (int)ve.size();++i) {
			cout<< ","<< ve[i].first<< ","<< ve[i].second.addr_length;
		}
		cout<<endl;
	}
	cout<< "---------end---------"<< endl;	
};

void bus_router::router_loginReq(dzh_bus_interface::Bus_Head* bus)
{
	dzh_bus_interface::LoginReq loginReq = bus->body().loginreq();
	if(!find_address_map(p_addr_)) {
		send_msg_back("already logined.");
		return;
	}

	addr_data adt;
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
				vector<addr_pair> addr_pair_vec = iter_serviceNo->second;
				addr_pair_vec.push_back(addr_pair(p_addr_,adt));
			}
			else {
				vector<addr_pair> addr_pair_vec;
				addr_pair_vec.push_back(addr_pair(p_addr_,adt));
				serviceNo_map_.insert(pair<unsigned int, vector<addr_pair>>(adt.ServiceNo_vector[i], addr_pair_vec));
			}
		}	
	}

	//print_map();

	//应答
	router_loginRsp(bus);
};

void bus_router::router_loginRsp(dzh_bus_interface::Bus_Head* bus)
{
	dzh_bus_interface::Bus_Head rep_bh;
	rep_bh.set_requestid(bus->requestid());
	rep_bh.set_endflag(bus->endflag());
	rep_bh.set_bodytype(2);
		
	dzh_bus_interface::Body rep_b;
	dzh_bus_interface::LoginRsp lgr;
	lgr.set_routerno(1);
	dzh_bus_interface::RspInfo rsp;
	rsp.set_rspno(0);
	rsp.set_rspdesc("login succeed.");

	lgr.set_allocated_rspinfo(&rsp);
	rep_b.set_allocated_loginrsp(&lgr);
	rep_bh.set_allocated_body(&rep_b);

	int size_buf;
	void* buf = Serialize(&rep_bh, size_buf);

	rsp.release_rspdesc();
	lgr.release_rspinfo();
	rep_b.release_loginrsp();
	rep_bh.release_body();

	zmq_send(router_socket_, p_addr_, addr_length_, ZMQ_SNDMORE);
	zmq_send(router_socket_, buf, size_buf, 0);
	
	free(buf);
};

void bus_router::router_logoutReq(dzh_bus_interface::Bus_Head* bus)
{
	//
};

void bus_router::router_logoutRsp()
{
	//
};

void bus_router::router_other_dealer(dzh_bus_interface::Bus_Head* bus)
{
	requestID_map::iterator iter_requestID = requestID_map_.find(requestID_pair(p_addr_, bus->requestid()));
	if(iter_requestID == requestID_map_.end()) {
		serviceNo_map::iterator iter_servieNo = serviceNo_map_.find(bus->bodytype());
		if(iter_servieNo == serviceNo_map_.end()) {
			send_msg_back("unregistered body type no.");
			return;
		}
		else {
			vector<addr_pair> addr_pair_vec = iter_servieNo->second;
			addr_pair addrp = addr_pair_vec[rand()%(addr_pair_vec.size())];   //随机发送

			add_requestID_map(addrp.first, bus->requestid());

			bus->set_requestid(requestID_);

			int size_buf = 0;
			void* buf = Serialize(bus, size_buf);

			send_msg((void*)addrp.first.c_str(), addrp.second.addr_length, buf, size_buf);

			free(buf);
		}			
	}
	else {
		addr_pair rep_addr = iter_requestID->second;
		bus->set_requestid(rep_addr.second.requestID);

		int size_buf = 0;
		void* buf = Serialize(bus, size_buf);

		send_msg((void*)rep_addr.first.c_str(), rep_addr.second.addr_length, buf, size_buf);

		free(buf);
	}
};

void* bus_router::Serialize(dzh_bus_interface::Bus_Head* bus, int& size)
{
	size = bus->ByteSize();
	void* buf = malloc(size);
	memset(buf, 0x00, size);
	bus->SerializeToArray(buf, size);	
	
	return buf;
};

void bus_router::add_requestID_map(string addr, unsigned int requestID)
{
	++requestID_;

	requestID_pair req_requestIDp(addr, requestID_);

	addr_data ad;
	ad.addr_length = addr_length_;
	ad.requestID = requestID;
	addr_pair req_addrp(p_addr_, ad);

	requestID_map_.insert(pair<requestID_pair, addr_pair>(req_requestIDp, req_addrp));
};

int bus_router::send_msg(void* addr, unsigned int addr_len, void* msg, unsigned int msg_len)
{
	zmq_send(router_socket_, addr, addr_len, ZMQ_SNDMORE);
	zmq_send(router_socket_, msg, msg_len, 0);

	return 0;
};

int bus_router::send_msg_back(char* msg)
{
	send_msg(p_addr_, addr_length_, msg, strlen(msg));

	return 0;
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

	//cout<< "--------------find address--------------"<< endl;
	//cout<< addr<< endl;

	addr_map::iterator iter_addr_map = addr_map_.find(addr);
	if(iter_addr_map == addr_map_.end())
		return -1;

	return 0;
};
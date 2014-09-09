#include <zmq.h>
#include <iostream>
#include <assert.h>

using namespace std;

//int main(int argc, char** argv){
//    void* context = zmq_ctx_new();
//    void* subscriber = zmq_socket(context, ZMQ_SUB);
//    zmq_setsockopt(subscriber, ZMQ_IDENTITY, "subscriber", 10);
//	assert(zmq_connect(subscriber, "tcp://localhost:36003") == 0);
//	
//    const char* filter = "xxx";
//    assert(zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, filter, strlen(filter)) == 0);
//    
//    while(true){
//        zmq_msg_t msg;
//        zmq_msg_init(&msg);
//        zmq_msg_recv(&msg, subscriber, 0);
//        printf("get msg: %s\n", zmq_msg_data(&msg));
//        zmq_msg_close(&msg);
//    }
//    
//    zmq_close(subscriber);
//    zmq_ctx_destroy(context);
//    return 0;
//}

int main(int argc, char *argv[])
{
	void *context = zmq_init (1);
	void* subscriber = zmq_socket(context, ZMQ_SUB);
	zmq_setsockopt(subscriber, ZMQ_IDENTITY, "subscriber1", 11);

    //zmq_connect(subscriber, "tcp://localhost:36003");
	zmq_connect(subscriber, "tcp://10.15.89.122:36002");

	zmq_setsockopt(subscriber,ZMQ_SUBSCRIBE,"",0);

	cout<< "listen pub."<< endl;

	char msg[256];

	while(1) {
		memset(msg,0x00,sizeof(msg));
		zmq_recv(subscriber,msg,sizeof(msg),0);
		printf("get msg: %s\n", msg);
	}

	zmq_close(subscriber);	
	zmq_ctx_destroy(context);

	system("pause");

	return 0;
};
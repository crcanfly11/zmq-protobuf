#include <zmq.h>
#include <iostream>
#include <assert.h>

using namespace std;

//int main(int argc, char** argv){
//    void* context = zmq_ctx_new();
//    void* publisher = zmq_socket(context, ZMQ_PUB);
//	zmq_setsockopt(publisher, ZMQ_IDENTITY, "publisher", 9);	
//    //assert(zmq_bind(publisher, "tcp://*:36004") == 0);
//	assert(zmq_connect(publisher, "tcp://localhost:36004") == 0);
//    
//    srand(0);
//    while(true){
//        char buf[64];
//        sprintf(buf, "xxx a=%d b=%d c=%d", rand(), rand(), rand());
//        
//        zmq_msg_t msg;
//        zmq_msg_init_size(&msg, strlen(buf)+1);
//        memcpy(zmq_msg_data(&msg), buf, strlen(buf)+1);
//        zmq_msg_send(&msg, publisher, 0);
//        zmq_msg_close(&msg);
//        
//        //Sleep(rand()%100 *1000);
//		Sleep(1000);
//    }
//    
//    zmq_close(publisher);
//    zmq_ctx_destroy(context);
//    return 0;
//}

int main(int argc, char* argv[])
{
    void *ctx = zmq_init(1);
    
	void* publisher = zmq_socket(ctx, ZMQ_PUB);
	zmq_setsockopt(publisher, ZMQ_IDENTITY, "publisher1", 10);
	//zmq_bind(publisher, "tcp://*:36004");
    //zmq_connect(publisher, "tcp://localhost:36004");
	zmq_connect(publisher, "tcp://10.15.89.122:36001");

	cout<< "service pub."<< endl;
	
	srand(0);
    while(1)
	{
        char buf_pub_a[64];
        sprintf(buf_pub_a, "xxx a=%d", rand());

        char buf_pub_b[64];
        sprintf(buf_pub_b, "xxx b=%d", rand());

		zmq_send(publisher,"pub_a",5,ZMQ_SNDMORE);
		zmq_send(publisher,buf_pub_a,strlen(buf_pub_a)+1,0);

		zmq_send(publisher,"pub_b",5,ZMQ_SNDMORE);
		zmq_send(publisher,buf_pub_b,strlen(buf_pub_b)+1,0);

		Sleep(1000);
    }

	zmq_close(publisher);
    zmq_ctx_destroy(ctx);

	system("pause");

    return 0;	
	
}
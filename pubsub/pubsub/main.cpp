#include <zmq.h>
#include <assert.h>

//int main(int argc, char** argv){
//    void* context = zmq_ctx_new();
//    void* frontend_xsub = zmq_socket(context, ZMQ_XSUB);
//    void* backend_xpub = zmq_socket(context, ZMQ_XPUB);
//    
//    //assert(zmq_connect(frontend_xsub, "tcp://localhost:36004") == 0);
//	assert(zmq_bind(frontend_xsub, "tcp://*:36004") == 0);
//    assert(zmq_bind(backend_xpub, "tcp://*:36003") == 0);
//    
//    zmq_proxy(frontend_xsub, backend_xpub, NULL);
//    
//    zmq_close(backend_xpub);
//    zmq_close(frontend_xsub);
//    zmq_ctx_destroy(context);
//    return 0;
//}

int main(int argc, char *argv[])
{
	void *context = zmq_init (1);
	void *xsub = zmq_socket(context, ZMQ_XSUB);
	zmq_bind(xsub, "tcp://*:36004");
	//zmq_connect(xsub, "tcp://localhost:36001");

	void *xpub = zmq_socket(context, ZMQ_XPUB);
	zmq_bind(xpub, "tcp://*:36003");

	zmq_proxy(xsub, xpub, NULL);

	zmq_close(xsub);
	zmq_close(xpub);
	zmq_ctx_destroy(context);

	system("pause");

	return 0;
};
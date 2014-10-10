#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <memory>
#include <sys/time.h>
#include <zookeeper.h>
#include "yt/log/ostreamlogger.h"
#include "yt/log/log.h"
#include "yt/log/datefilelogger.h"
#include "serverconf.h"
#include "bus_router.h"

#define MAX_CLIENT_COUNT 1024
#define MAX_ONE_PACKAGE_LEN 1024*1024

using namespace yt;

bool end_process = false;
zhandle_t* zkhandle;
ServerConf g_conf;

void* subandpub(void*);
void signal_manage(int signo);
int zookeeper_manage(); 
void zookeeper_watcher(zhandle_t* zh, int type, int state, const char* path, void* watcherCtx);

int main(int argc, char **argv)
{
	char configfilename[128];// modualname[128];
	memset(configfilename,0,sizeof(configfilename));
	int ch;
	bool isdaemon = false;

	while((ch = getopt(argc,argv,"f:d"))!= -1)
	{
		switch(ch)
		{
			case 'f':
				{
					strncpy(configfilename,optarg,sizeof(configfilename));
				}
				break;
			case 'd':
				{
					isdaemon = true;
				}
				break;
		}
	}
        if(strlen(configfilename) == 0)
                strcpy(configfilename,"../etc/router.xml");
	cout << "configfilename " << configfilename << endl;

	if(isdaemon)
	{
		int pid;
		signal(SIGCHLD, SIG_IGN);
		pid = fork();
		if(pid < 0) {
			perror("fork");
			exit(-1);
		}
		else if(pid > 0)
			exit(0);
		setsid();
	}
	signal(SIGCHLD, SIG_DFL);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, signal_manage);
	signal(SIGKILL, signal_manage);
	signal(SIGTERM, signal_manage);
	signal(SIGALRM, signal_manage);
	
	auto_ptr<OStreamLogger> stdoutlog(new OStreamLogger(std::cout));
	AC_SET_DEFAULT_LOGGER(stdoutlog.get());
	DateFileLogger *runlog = NULL;
	DateFileLogger *errlog = NULL;

	if (!g_conf.ReadCfg(configfilename))
	{
		std::cout<< "ReadCfg() failed" << std::endl;
		return 0;
	}

	if(isdaemon)
	{	
		runlog = new DateFileLogger(g_conf.m_logdir + "router" + "runlog");
		errlog = new DateFileLogger(g_conf.m_logdir + "router" + "syslog");

		AC_SET_DEFAULT_LOGGER(errlog);
		AC_SET_LOGGER(LP_USER1,runlog);
	}
	//AC_INFO("hello info");
	//AC_DEBUG("hello debug");
	//AC_ERROR("hello error");

	auto_ptr<DateFileLogger> g(runlog);
	auto_ptr<DateFileLogger> g2(errlog);

	//run zookeeper
	zookeeper_manage();

	//run zmq
	pthread_t tid;
	if(pthread_create(&tid, NULL, subandpub, NULL)!=0) {
		AC_ERROR("pthread create error.");
	}

	void* context = zmq_init(1);
	void* socket = zmq_socket(context, ZMQ_ROUTER);
	
	if(!g_conf.m_strName.empty()){
		zmq_setsockopt(socket, ZMQ_IDENTITY, g_conf.m_strName.c_str(), g_conf.m_strName.size());	
	}	

	if(zmq_bind(socket, g_conf.m_endpoint.c_str()) == 0)
		AC_INFO("binded.");
	else {
		AC_ERROR("bind error");

		zmq_close(socket);
		zmq_ctx_destroy(context);

		return 0;
	}

	bus_router rt(socket);
	rt.run();

	zmq_close(socket);
	zmq_ctx_destroy(context);

	if(zoo_delete(zkhandle, g_conf.m_nodepath.c_str(), -1) == ZOK)
		AC_INFO("delete node success");


	return 0;
}

void signal_manage(int signo)
{
	switch(signo) {
		case SIGINT:
		case SIGKILL:
		case SIGTERM:
			signal(SIGINT, SIG_IGN);
        		signal(SIGKILL, SIG_IGN);
        		signal(SIGTERM, SIG_IGN);
			end_process = true;
			break;
		default:
			break;
	}
}

void* subandpub(void*)
{
	void* context = zmq_init(1);
	void* xsub = zmq_socket(context, ZMQ_XSUB);
	zmq_bind(xsub, g_conf.m_subpoint.c_str());
	void* xpub = zmq_socket(context, ZMQ_XPUB);
	zmq_bind(xpub, g_conf.m_pubpoint.c_str());

	zmq_proxy(xsub, xpub, NULL);
	
	zmq_close(xsub);
	zmq_close(xpub);
	zmq_ctx_destroy(context);
	
	return NULL;
}

int zookeeper_manage()
{
	zoo_set_debug_level(ZOO_LOG_LEVEL_WARN); //set log

	zkhandle = zookeeper_init(g_conf.m_zookeeperhost.c_str(), zookeeper_watcher, g_conf.m_zookeepertimeout, 0, (void*)g_conf.m_businesspath.c_str(), 0);

	if(zkhandle ==NULL) 
	{
		AC_ERROR("Error when connecting to zookeeper servers...");
		return -1;
	}
	
	char node_buf[64];

	string str(" { \"idc\" : ");
	str.append(g_conf.m_idc);
	str.append(", \"info\" : { \"apptype\" : 3, \"businesspath\" : \"");
	str.append(g_conf.m_businesspath);
	str.append("\", \"appinterface\" : \"");
	str.append(g_conf.m_appinterface);
	str.append("\"}, \"userdata\" : \"\"}");

	int flag = zoo_create(zkhandle, g_conf.m_nodepath.c_str(), str.c_str(), str.size(), &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, node_buf, sizeof(node_buf)); 
	
	if(flag != ZOK) {
		AC_ERROR("Error create node.");
		return -1;
	}
	else 
		AC_INFO("creat new node: %s", node_buf);

	return 0;
};

void zookeeper_watcher(zhandle_t* /*zh*/, int type, int state, const char* path, void* watcherCtx)
{
	AC_INFO("type: %d", type);
	AC_INFO("state: %d", state);
	AC_INFO("path: %s", path);
	AC_INFO("watcherCtx: %s", (char*)watcherCtx);
};

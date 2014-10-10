#pragma once

#include <vector>
#include <string>
#include "yt/xml/tinyxml.h"

using namespace std;

class ServerConf
{
	public:
		bool ReadCfg(const char *configfile);

		string m_logdir;
		string m_strName;
		string m_endpoint;
		string m_subpoint;
		string m_pubpoint;
		int m_replyouttime;
		int m_heartbeatouttime;
		int m_msgmaxlen;
		string m_idc;
		string m_businesspath;
		string m_appinterface;
		string m_zookeeperhost;
		int m_zookeepertimeout;
		string m_nodepath;
};

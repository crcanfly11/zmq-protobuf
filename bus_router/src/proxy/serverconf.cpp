#include "serverconf.h"
#include "yt/log/log.h"

bool ServerConf::ReadCfg(const char* configfile)
{
	TiXmlDocument doc(configfile);
	bool ret = doc.LoadFile();
	if (!ret)
	{
		AC_ERROR("configfile:%s load fail",configfile);
		return false;
	}
	TiXmlElement *root = doc.FirstChildElement("config");
	if(!root)
	{
		AC_ERROR("read config error");
		return false;
	}
	TiXmlElement *cfg = root->FirstChildElement("cfg");
	if(!cfg)
	{
		AC_ERROR("read cfg error");
		return false;
	}
	
	const char *str = cfg->Attribute("logdir");
	if(!str)
	{
		AC_ERROR("read logdir error");
		return false;
	}
	AC_INFO("logdir  = %s",str);
	m_logdir = str;

	str = cfg->Attribute("name");
	if(!str)
	{
		AC_ERROR("read name error");
		return false;
	}
	AC_INFO("name  = %s",str);
	m_strName = str;

	str = cfg->Attribute("subpoint");
	if(!str)
	{
		AC_ERROR("read subpoint error");
		return false;
	}
	AC_INFO("subpoint = %s",str);
	m_subpoint = str;

	str = cfg->Attribute("pubpoint");
	if(!str)
	{
		AC_ERROR("read pubpoint error");
		return false;
	}
	AC_INFO("pubpoint = %s",str);
	m_pubpoint = str;

	str = cfg->Attribute("idc");
	if(!str)
	{
		AC_ERROR("read idc error");
		return false;
	}
	AC_INFO("idc  = %s",str);
	m_idc = str;
	
	str = cfg->Attribute("businesspath");
	if(!str)
	{
		AC_ERROR("read businesspath error");
		return false;
	}
	AC_INFO("businesspath  = %s",str);
	m_businesspath = str;

	str = cfg->Attribute("appinterface");
	if(!str)
	{
		AC_ERROR("read appinterface error");
		return false;
	}
	AC_INFO("appinterface = %s",str);
	m_appinterface = str;

	str = cfg->Attribute("zookeeperhost");
	if(!str)
	{
		AC_ERROR("read zookeeperhost error");
		return false;
	}
	AC_INFO("zookeeperhost = %s",str);
	m_zookeeperhost = str;

	str = cfg->Attribute("zookeepertimeout");
	if(!str)
	{
		AC_ERROR("read zookeepertimeout error");
		return false;
	}
	AC_INFO("zookeepertimeout = %s",str);
	m_zookeepertimeout = atoi(str);

	str = cfg->Attribute("nodepath");
	if(!str)
	{
		AC_ERROR("read nodepath error");
		return false;
	}
	AC_INFO("nodepath = %s",str);
	m_nodepath = str;
	
	return true;
}

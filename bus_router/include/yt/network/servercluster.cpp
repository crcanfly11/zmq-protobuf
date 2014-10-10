#include "yt/network/servercluster.h"
#include "yt/util/md5.h"
#include "json.h"
#include "yt/util/jsonobjguard.h"


namespace yt
{
	void Servers::AddServer(ServerSocketBase *server)
	{
		m_Serverlist.push_back(server);
	}
	void Servers::Close()
	{
		for(int i = 0;i < (int)m_Serverlist.size();i++)
		{
			m_Serverlist[i]->RealClose();
			//delete m_Serverlist[i];
		}
		m_Serverlist.clear();
	}
	void Servers::GetStatus(json_object *arryobj)
	{
		for(int i = 0;i < (int)m_Serverlist.size();i++)
		{
			json_object *myobj = json_object_new_object();
			m_Serverlist[i]->GetStatus(myobj);
			json_object_array_add(arryobj,myobj);
		}
	}
	ServerSocketBase* StateServers::GetServer(const char*,size_t)
	{
		static int i = 0;
		static ThreadMutex mutex;
		mutex.Acquire();
		LockGuard g(&mutex);

		bool flag = false;
		for(size_t j = 0;j < m_Serverlist.size();j++)
		{
			if(i++ > 500000)
				i = 0;
			int index = i % m_Serverlist.size();
			StateTcpServerSocket *server = (StateTcpServerSocket*)m_Serverlist[index];
			if(!server->IsOk())
			{
				char desc[64];
				server->GetDesc(desc);
			}
			if(!server->IsConnect())	//连不上
			{
				flag = true;
				continue;
			}
			else
			{
				if(server->IsOk())
					return server;
			}
		}
		if(flag == false)	//都连得上，但是都不同步状态，就随机取一台即可
		{
			srand(time(NULL));
			int index = rand() % m_Serverlist.size();
			StateTcpServerSocket *server = (StateTcpServerSocket*)m_Serverlist[index];
			return server;
		}
		return NULL;
	}
	ServerSocketBase* Servers::GetServer(const char *key,size_t keylen)
	{
		if(m_Serverlist.size() == 0)
			return NULL;
		switch(m_type)
		{
			case HASH:
				{
					int index;
					if(m_Serverlist.size() == 1)
						index = 0;
					else
					{
						long hashvalue = GetHashValue(key,keylen);
						if(hashvalue == -1)
							return NULL;
						index = hashvalue % m_Serverlist.size();
					}
					ServerSocketBase *server = m_Serverlist[index];
					if(server->IsConnect())
						return server;
					else
						return NULL;
				}
				break;
			case RAND:
				{
					int i = rand() % m_Serverlist.size();
					for(size_t j = i;j < i + m_Serverlist.size();j++)
					{
						ServerSocketBase *server = m_Serverlist[j % m_Serverlist.size()];
						if(!server->IsConnect())
						{
							continue;
						}
						else
						{
							return server;
						}
					}
					return NULL;
				}
				break;
			case LOADBALANCE:
				{
					if(m_Serverlist.size() == 1)
					{
						ServerSocketBase *server = m_Serverlist[0];
						if(server->IsConnect())
							return server;
						else
							return NULL;
					}
					static const int MAXNORMALGSSISPEED = 5000;//5毫秒以内的都算正常
					int minload,load;
					size_t i,index;
					for(i = 0;i < m_Serverlist.size();i++)//先取出一台连接状态正常的服务器的速度
					{
						if(m_Serverlist[i]->IsConnect())
						{
							if(m_Serverlist[i]->GetGSSISpeed() < MAXNORMALGSSISPEED)
								minload = m_Serverlist[i]->GetServerLoad();
							break;
						}
					}
					if(i == m_Serverlist.size())
						return NULL;

					index = i;
					i = index + 1;
					for(;i < m_Serverlist.size();i++)
					{
						if(m_Serverlist[i]->IsConnect())
						{
							if(m_Serverlist[i]->GetGSSISpeed() < MAXNORMALGSSISPEED)
								continue;
							load = m_Serverlist[i]->GetServerLoad();
							if(load < minload)
							{
								minload = load;
								index = i;
							}
						}
					}
					
					return m_Serverlist[index];
				}
				break;
			default:
				break;
		}

		return NULL;
	}
	long Servers::GetHashValue(const char *key,size_t keylen)
	{
		switch(m_hashtype)
		{
			case MD5:
				{
					//char md5key[33];
					//md5_string((const unsigned char*)key,md5key,keylen);
					//md5key[2] = 0;
					if(keylen != 32)
						assert(0);
					char md5key[8];
					memcpy(md5key,key,8);
					md5key[4] = 0;

					return strtol(md5key,NULL,16);
				}
				break;
			case NUMMOD:
				{
					int num = *( (int*)key );
					return num;
				}
				break;
			default:
				break;
		}
		return -1;
	}
	ServerSocketBase* ServerCluster::GetServer(int svrid,const char *key,size_t keylen)
	{
		Servers **servers = Get(svrid);
		if(!servers)
			return NULL;
		return (*servers)->GetServer(key,keylen);
	}
	void ServerCluster::Close()
	{
		for(iterator it = Begin();it != End();it++)
		{
			it->second->Close();
			delete it->second;
		}
		Clear();
	}
	/*Servers* ServerCluster::CreateServers(int svrid,const char *type,const char *hashtype)
	{
		Servers *svrs = new(nothrow) Servers(type,hashtype);
		if(!svrs)
			return NULL;
		Put(svrid,svrs);
		return svrs;
	}*/
	void ServerCluster::GetStatus(json_object *arrobj)
	{
		for(iterator it = Begin();it != End();it++)
		{
			//json_object *myarrobj = json_object_new_array();
			it->second->GetStatus(arrobj);
			//json_object_array_add(arrobj,myarrobj);
		}
	}
	ServerSocketBase* ServerClusterMap::GetServer(int scid,int svrid,const char *key,size_t keylen)
	{
		ServerCluster **sc = Get(scid);
		if(!sc)
			return NULL;
		return (*sc)->GetServer(svrid,key,keylen);
	}
	void ServerClusterMap::Close()
	{
		for(iterator it = Begin();it != End();it++)
		{
			it->second->Close();
			delete it->second;
		}
		Clear();
	}
	/*ServerCluster* ServerClusterMap::CreateSC(int scid)
	{
		ServerCluster *sc = new(nothrow) ServerCluster;
		if(!sc)
			return NULL;
		Put(scid,sc);
		return sc;
	}*/
	void ServerClusterMap::GetStatus(json_object *arrobj)
	{
		for(iterator it = Begin();it != End();it++)
		{
			it->second->GetStatus(arrobj);
		}
	}
}


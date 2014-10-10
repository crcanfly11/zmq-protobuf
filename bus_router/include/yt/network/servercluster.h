#pragma once

#include "yt/util/seqmap.h"
#include "yt/network/serversocket.h"
#include "yt/xml/tinyxml.h"
#include <vector>
#include <json.h>

using namespace std;

namespace yt
{
	//一些服务器集群类，选择服务器用	
	class Servers
	{
		enum TYPE
		{
			NULLT = 0x0,
			HASH = 0x01,	//有主备
			RAND,		//随机取一台
			LOADBALANCE,	//取负载最轻的一台
		};
		enum HASHTYPE
		{
			NULLHT = 0x0,
			MD5 = 0x1,	//md5
			NUMMOD,		//数字求余
		};
		public:
			Servers(const char *type,const char *hashtype = NULL) : m_type(NULLT),m_hashtype(NULLHT)
			{
				if(strcmp(type,"hash") == 0)
					m_type = HASH;
				else if(strcmp(type,"rand") == 0)
					m_type = RAND;
				else if(strcmp(type,"loadbalance") == 0)
					m_type = LOADBALANCE;
				if(m_type == HASH)
				{
					if(hashtype)
					{
						if(strcmp(hashtype,"md5") == 0)
							m_hashtype = MD5;
						else if(strcmp(hashtype,"nummod") == 0)
							m_hashtype = NUMMOD;
					}
				}
			}
			virtual ~Servers(){}
			virtual long GetHashValue(const char *key,size_t keylen);
			virtual ServerSocketBase* GetServer(const char *key,size_t keylen);
			void AddServer(ServerSocketBase *server);
			void Close();
			inline void SetDesc(const char *desc){
				m_desc.clear();
				m_desc.append(desc);
			}
			inline string& GetDesc(){
				return m_desc;
			}
			inline const vector<ServerSocketBase*>& GetServerlist() const{
                                return m_Serverlist;
                        }
			void GetStatus(json_object *arryobj);
		protected:
			vector<ServerSocketBase*> m_Serverlist;
		private:
			TYPE m_type;
			HASHTYPE m_hashtype;
			string m_desc;
	};
	class StateServers : public Servers
	{
		public:
			StateServers(const char *type) : Servers(type,NULL)
			{
				if(strcmp(type,"rand") != 0)
					assert(0);
			}
			virtual ServerSocketBase* GetServer(const char *key,size_t keylen);
	};
	class ServerCluster : public SeqMap<Servers*>
	{
		public:
			virtual ServerSocketBase* GetServer(int svrid,const char *key,size_t keylen);
			//Servers* CreateServers(int svrid,const char *type,const char *hashtype = NULL);
			void Close();
			void GetStatus(json_object *obj);
			virtual ~ServerCluster(){}
		private:
	};
	class ServerClusterMap : public SeqMap<ServerCluster*>
	{
		public:
			virtual ServerSocketBase* GetServer(int scid,int svrid,const char *key,size_t keylen);
			virtual ~ServerClusterMap(){Close();}
			//ServerCluster* CreateSC(int scid);
			void GetStatus(json_object *arrobj);
		private:
			void Close();
	};
	class ServerCluster2 : public ServerCluster
	{
		public:
			void AddId(int id,int svrid){m.insert(make_pair(id,svrid));}
			virtual ServerSocketBase* GetServer(int id,const char *key,size_t keylen){
				if(m.find(id) == m.end())
					return NULL;
				return ServerCluster::GetServer(m[id],key,keylen);
			}
		private:
			map<int,int> m;
	};
	class ServerClusterMap2 : public ServerClusterMap
	{
		public:
			void AddId(int id,int scid){m.insert(make_pair(id,scid));}
			virtual ServerSocketBase* GetServer(int id,int svrid,const char *key,size_t keylen){
				if(m.find(id) == m.end())
					return NULL;
				return ServerClusterMap::GetServer(m[id],svrid,key,keylen);
			}
		//private:
			map<int,int> m;
	};
}


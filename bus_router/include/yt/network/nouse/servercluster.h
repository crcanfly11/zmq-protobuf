#ifndef AC_SERVERCLUSTER_H_
#define AC_SERVERCLUSTER_H_

#include <vector>
#include "./network/xmlcfgbase.h"
#include "./network/tmbackclientsocket.h"

using namespace ac;
template <class T> T* GetServer(const vector<T*> *serverlist,const ClusterCfgBase *clustercfg,const char* key,size_t keylen);
namespace ac
{
	class MBClient : public TMBackClientSocketBase
	{
		public:
			MBClient(Reactor *pReactor,const char* ip,int port,bool bMainOrBak = true) : 
				TMBackClientSocketBase(pReactor,ip,port),m_bMainOrBak(bMainOrBak){}
			bool IsMain(){return m_bMainOrBak;}
			bool IsBak(){return !m_bMainOrBak;}
		private:
			bool m_bMainOrBak;
	};

	template <class T = TMBackClientSocketBase>
		class ServerCluster : public StatusQuery
		{
			public:
				virtual void UnInit()
				{
					for(int i = 0;i < (int)m_MainServerList.size();i++)
					{
						m_MainServerList[i]->RealClose();
						delete m_MainServerList[i];
					}

					for(int i = 0;i < (int)m_BakServerList.size();i++)
					{
						m_BakServerList[i]->RealClose();
						delete m_BakServerList[i];
					}
					m_MainServerList.clear();
					m_BakServerList.clear();
				}
				virtual ~ServerCluster(){}
				virtual T* GetMainServer(const char *key,size_t keylen) = 0;
				virtual T* GetBakServer(const char* key,size_t keylen) = 0;
				virtual void GetInfo(char* str)
				{
					for(int i = 0;i < (int)m_MainServerList.size();i++)
					{
						m_MainServerList[i]->GetInfo(str);
						strcat(str,"\n");
					}

					for(int i = 0;i < (int)m_BakServerList.size();i++)
					{
						m_BakServerList[i]->GetInfo(str);
						strcat(str,"\n");
					}
				}
			protected:
				vector<T*> m_MainServerList;
				vector<T*> m_BakServerList;
		};
}

#endif

#include "yt/network/clientmap.h"
#include "yt/network/clientsocket.h"

namespace yt
{	
	void ClientMap::Close()
	{
		vector<ClientSocket*> v;
		for(const_iterator it = Begin();it != End();it++)
			v.push_back(it->second);

		for(size_t i = 0;i < v.size();i++)
			v[i]->Close();

		Clear();
	}
	ClientSocket* ClientMap::Get(int seq)
	{
		ClientSocket** pClient = SeqMap<ClientSocket*>::Get(seq);
		if(pClient == NULL)
		{
			return NULL;
		}
		return *pClient;
	}
	void ClientMap::SendAll(const char *buf,size_t buflen)
	{
		/*vector<ClientSocket*> v;
		for(const_iterator it = Begin();it != End();it++)
			v.push_back(it->second);

		for(size_t i = 0;i < v.size();i++)
		{
			if(v[i]->SendBuf(buf,buflen) != 0)
				v[i]->Close();
		}*/
		iterator it;
		ClientSocket *clientsocket;
		for(it = Begin();it != End();)
		{
			clientsocket = it->second;
			if(clientsocket->SendBuf(buf,buflen) != 0)
			{
				it++;
				clientsocket->Close();
			}
			else
				it++;
		}
	}
}

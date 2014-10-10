#pragma once

#include <stdio.h>
#include <sys/time.h>
#include "yt/network/tmseqmap.h"
namespace yt
{
	class ReserveData;
	typedef TMSeqMap<ReserveData*> RDMAP;
	//异步保留数据的基类
	class ReserveData //: public FSMPoolableThread
	{
		public:
			ReserveData(RDMAP *seqmap,int _asyncseq,short _cmd,int _packseq,int _clientseq,int _type) :
				m_seqmap(seqmap),asyncseq(_asyncseq),cmd(_cmd),packseq(_packseq),clientseq(_clientseq),type(_type)
			{
				gettimeofday(&t,NULL);
				m_seqmap->Put(asyncseq,this);
				count = 1;
			}
			virtual ~ReserveData()
			{
				TMT2<ReserveData*> *pTmt=m_seqmap->GetTMT(asyncseq);
				time_t t;
				if(pTmt)
				{
					t=pTmt->ti;
					MinHeap<pair<time_t,int> > *minheap=m_seqmap->GetMinHeap();
					if(minheap)
					{
						minheap->Del(make_pair(t,asyncseq));
					}
				}
				m_seqmap->Del(asyncseq);
			}
			void ResetTime()
			{
				gettimeofday(&t,NULL);
			}
			void Reset()
			{
				if(m_seqmap)
				{
					TMT2<ReserveData*> *pTmt = m_seqmap->GetTMT(asyncseq);
					time_t old=pTmt->ti;
					pTmt->ti = time(NULL);
					MinHeap<pair<time_t,int> > *minheap=m_seqmap->GetMinHeap();
					if(minheap)
					{
						minheap->Del(make_pair(old,asyncseq));
						minheap->Insert(make_pair(pTmt->ti,asyncseq));
					}
					count++;
				}
			}
			virtual string GetInfo()
			{
				char info[128];
				sprintf(info,"cmd = %d,asyncseq = %d,packseq = %d,clientseq = %d,type = %d.",cmd,asyncseq,packseq,clientseq,type);
				return info;
			}
			/*void SetTimeout(int timeout2){
				timeout = timeout2;
			}*/

			RDMAP *m_seqmap;
			int asyncseq;

			short cmd;
			int packseq;
			int clientseq;

			int type;
			timeval t;

			int count;

			//int timeout;			
	};
	/*class ReserveDataGuard
	{
		public:
			ReserveDataGuard(ReserveData *rd) : m_rd(rd){}
			~ReserveDataGuard(){
				if(m_rd->count <= 0)
				{
					delete m_rd;
				}
			}
		private:
			ReserveData *m_rd;
	};*/
	class ReserveDataGuard
	{
		public:
			ReserveDataGuard(RDMAP *seqmap,int asyncseq) : m_seqmap(seqmap),m_asyncseq(asyncseq){}
			~ReserveDataGuard(){
				ReserveData **rd = m_seqmap->Get(m_asyncseq);
				if(!rd)
					return;
				ReserveData *rd2 = *rd;
				if(rd2->count <= 0)
					delete rd2;
			}
		private:
			RDMAP *m_seqmap;
			int m_asyncseq;
	};

	const static int RDTYPE_GETSERVERSYSINFO = -100;
}

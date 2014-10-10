#pragma once 

#include <set>

#include "yt/util/queue.h"
#include "yt/util/thread.h"

using namespace yt;
using namespace std;

namespace yt
{
#define DEFAULT_SKIP_TIME	1

	class InfoUploadProcessorBase;

	typedef struct
	{
		int   m_type;
		void *m_data;	
	} InfoStruct_t;

	typedef struct
	{
		int m_interval;
		int m_current;
		InfoUploadProcessorBase *m_processor;
		set<int> m_infotypes;
	} UploadProcessor_t;

	typedef void (*DELETEINFOFUN) (void*);

	class InfoUploadManager : public Thread
	{
		private:
			InfoUploadManager();
			virtual ~InfoUploadManager();

		public:
			static InfoUploadManager* Instance()
			{
				static InfoUploadManager IUM;
				return &IUM;
			}

			virtual void Run();
			void Stop();

			bool SendInfo(InfoStruct_t& info);

			void RegistDeleteInfoFun(int type, DELETEINFOFUN fun);

			bool RegistUploadProcessor(int uploadtype, InfoUploadProcessorBase *processor, int uploadinterval);
			bool UnRegistUploadProcessor(int uploadtype);

			bool RegistUploadInfoType(int uploadtype, int infotype);
			bool UnRegistUploadInfoType(int uploadtype, int infotype);

		private:
			void UploadInfo(int skip_time = DEFAULT_SKIP_TIME);
			void ClearQueue();

		private:
			bool m_run;
			SyncQueue<InfoStruct_t> m_infoqueue;

			map<int, DELETEINFOFUN> m_deleteinfofunset;
			map<int, UploadProcessor_t> m_uploadprocessorset;

	};

}

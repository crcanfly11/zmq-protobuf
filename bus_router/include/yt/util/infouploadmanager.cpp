#include "infouploadmanager.h"

#include "yt/log/log.h"
#include "yt/network/alarm.h"

#include "infouploadprocessorbase.h"
namespace yt
{
#define DEFAULT_UPLOADINTERVAL	1
#define MAX_QUEUESIZE			100000

	InfoUploadManager::InfoUploadManager()
	{
		m_infoqueue.SetLimit(MAX_QUEUESIZE);
	}

	InfoUploadManager::~InfoUploadManager()
	{
	}

	void InfoUploadManager::Run()
	{
		m_run = true;
		while(m_run)
		{
			// process info
			InfoStruct_t info;
			for (size_t cur_size = m_infoqueue.GetSize(); cur_size > 0; cur_size--)
			{
				m_infoqueue.Pop(info, true);
				if (info.m_data != NULL)
				{
					map<int, UploadProcessor_t>::iterator it_processor = m_uploadprocessorset.begin();
					for (; it_processor != m_uploadprocessorset.end(); it_processor++)
					{
						if (it_processor->second.m_infotypes.find(info.m_type) != it_processor->second.m_infotypes.end())
						{
							it_processor->second.m_processor->ProcessInfo(info);
						}
					}

					map<int, DELETEINFOFUN>::iterator it_deletefun = m_deleteinfofunset.find(info.m_type);
					if (it_deletefun != m_deleteinfofunset.end())
					{
						it_deletefun->second(info.m_data);
					}
					else
					{
						AC_ERROR("Can not find delete function for info type : %d, MEMORY LEAK!!!!!", info.m_type);
					}
				}
			}

			// upload info
			UploadInfo();

			sleep(1);
		}
	}

	void InfoUploadManager::Stop()
	{
		m_run = false;
	}

	bool InfoUploadManager::SendInfo(InfoStruct_t& info)
	{
		if (m_infoqueue.Push(info) == 0)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	void InfoUploadManager::RegistDeleteInfoFun(int type, DELETEINFOFUN fun)
	{
		m_deleteinfofunset[type] = fun;
	}

	bool InfoUploadManager::RegistUploadProcessor(int uploadtype, InfoUploadProcessorBase *processor, int uploadinterval)
	{
		map<int, UploadProcessor_t>::iterator it_processor = m_uploadprocessorset.find(uploadtype);
		if (it_processor == m_uploadprocessorset.end())
		{
			// add new uploadprocessor
			AC_INFO("Add new uploadprocessor, type : %d", uploadtype);
			UploadProcessor_t up;
			up.m_interval  = uploadinterval;
			up.m_current   = 0;
			up.m_processor = processor;
			m_uploadprocessorset[uploadtype] = up;
			return true;
		}
		else
		{
			AC_ERROR("Fail to regist uploadprocessor, type : %d, (already exist)", uploadtype);
			return false;
		}
	}

	bool InfoUploadManager::UnRegistUploadProcessor(int uploadtype)
	{
		map<int, UploadProcessor_t>::iterator it_processor = m_uploadprocessorset.find(uploadtype);
		if (it_processor != m_uploadprocessorset.end())
		{
			// add new uploadprocessor
			AC_INFO("erase uploadprocessor, type : %d", uploadtype);
			m_uploadprocessorset.erase(it_processor);
		}
		return true;
	}

	bool InfoUploadManager::RegistUploadInfoType(int uploadtype, int infotype)
	{
		map<int, UploadProcessor_t>::iterator it_processor = m_uploadprocessorset.find(uploadtype);
		if (it_processor != m_uploadprocessorset.end())
		{
			// add new infotype
			AC_INFO("Add new infotype %d to upload type : %d", infotype, uploadtype);
			it_processor->second.m_infotypes.insert(infotype);
			return true;
		}
		else
		{
			AC_ERROR("Can not find uploadprocessor with uploadtype %d, fail to add info type %d", uploadtype, infotype);
			return false;
		}
	}

	bool InfoUploadManager::UnRegistUploadInfoType(int uploadtype, int infotype)
	{
		map<int, UploadProcessor_t>::iterator it_processor = m_uploadprocessorset.find(uploadtype);
		if (it_processor != m_uploadprocessorset.end())
		{
			// erase infotype
			AC_INFO("delete infotype %d to upload type : %d", infotype, uploadtype);
			it_processor->second.m_infotypes.erase(infotype);
			return true;
		}
		else
		{
			AC_ERROR("Can not find uploadprocessor with uploadtype %d, fail to add info type %d", uploadtype, infotype);
			return false;
		}
	}

	// private functions

	void InfoUploadManager::UploadInfo(int skip_time)
	{
		map<int, UploadProcessor_t>::iterator it_processor = m_uploadprocessorset.begin();
		for (; it_processor != m_uploadprocessorset.end(); it_processor++)
		{
			it_processor->second.m_current += skip_time;
			if (it_processor->second.m_current >= it_processor->second.m_interval)
			{
				// upload info
				it_processor->second.m_current = 0;
				string jsonstr;
				if (it_processor->second.m_processor->CreateJson(jsonstr))
				{
					AC_INFO("Upload Info: %s", jsonstr.c_str());
					InfoPush(jsonstr);
				}
				else
				{
					AC_ERROR("Can not create json with type : %d", it_processor->first);
				}
			}
		}
	}

	void InfoUploadManager::ClearQueue()
	{
		InfoStruct_t info;
		while (m_infoqueue.GetSize() > 0)
		{
			m_infoqueue.Pop(info, true);
			if (info.m_data != NULL)
			{
				map<int, DELETEINFOFUN>::iterator it_deletefun = m_deleteinfofunset.find(info.m_type);
				if (it_deletefun != m_deleteinfofunset.end())
				{
					it_deletefun->second(info.m_data);
				}
				else
				{
					AC_ERROR("Can not find delete function for info type : %d, MEMORY LEAK!!!!!", info.m_type);
				}
			}
		}
	}

}



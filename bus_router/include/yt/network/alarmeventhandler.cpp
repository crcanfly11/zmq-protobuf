#include "yt/network/alarmeventhandler.h"
#include "yt/network/alarm.h"
#include "yt/log/log.h"
#include "json.h"
#include "yt/util/jsonobjguard.h"
#include "string.h"

namespace yt
{
	void AlarmEventHandlerBase::OnFDRead()
	{
		char buf[64];
		recv(m_fd,buf,sizeof(buf),0);
		//AC_INFO("%s",buf);
		while(1)
		{
		
			AlarmInfo alarminfo;
			bool isBreak = false;
			if(g_alarm->m_alarmqueue.Pop(alarminfo,false) == 0)

				OnAlarm(&alarminfo);
			else
				isBreak = true;
			string info;
			if(g_alarm->m_infoqueue.Pop(info,false) == 0)
				OnAlarm2(info);
			else
			{
				if(isBreak)
					break;
			}

		}
	}
	void AlarmEventHandler::OnAlarm(AlarmInfo *info)
	{
		json_object *alarmobj;

		alarmobj = json_object_new_object();
		JsonObjGuard g(alarmobj);
		json_object_object_add(alarmobj, "type", json_object_new_int(info->type));
		json_object_object_add(alarmobj, "level", json_object_new_int(info->level));
		json_object_object_add(alarmobj, "desc", json_object_new_string(info->info.c_str()));

		const char *str = json_object_to_json_string(alarmobj);
		size_t len = strlen(str);

		//AC_INFO("alarm = %s",str);
		JsonHeader jheader;
		jheader.m_len = ntohl(sizeof(JsonHeader) + len);
		jheader.m_cmd = ntohs((short)6);
		jheader.m_seq = ntohl(0);

		m_pClientMap->SendAll((char*)&jheader,sizeof(JsonHeader));
		m_pClientMap->SendAll(str,len);
	}
	void AlarmEventHandler::OnAlarm2(string & info)
	{
		JsonHeader jheader;
		jheader.m_len = ntohl(sizeof(JsonHeader) + info.size());
		jheader.m_cmd = ntohs((short)14);
		jheader.m_seq = ntohl(0);

		m_pClientMap->SendAll((char*)&jheader,sizeof(JsonHeader));
		m_pClientMap->SendAll(info.c_str(),info.size());
	}
}

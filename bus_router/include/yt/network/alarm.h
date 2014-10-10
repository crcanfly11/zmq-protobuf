#pragma once

#include "yt/network/eventhandler.h"
#include "yt/util/queue.h"

namespace yt
{
        #define AD_PROCESSQUEUEFULL "process queue full"
        #define AD_RESULTQUEUEFULL "result queue full"
	#define AD_CLIENTOVERFLOW "client over flow"

	static const short AT_PROCESSQUEUEFULL = 1;
	static const short AT_RESULTQUEUEFULL = 2;
	static const short AT_CLIENTOVERFLOW = 3;
	static const short AT_SERVERCONNERROR = 4;
	static const short AT_PACKTIMEOUT = 5;
	static const short AT_MEMERROR = 6;
	static const short AT_SENDBUFFULL = 7;
	static const short AT_RECVBUFFULL = 8;
	static const short AT_WRITECACHEERROR = 9;
	static const short AT_MYSQLTIMEOUT = 10;
	static const short AT_MYSQLERROR = 11;
	static const short AT_WRITEKVDBERROR = 12;
	static const short AT_APPENDKVDBERROR = 13;
	static const short AT_DATAERROR1 = 14;
	//static const short AT_UNSYNCSTATE = 21;

	static const short AT_DIDSTATISTICS = 15;
	static const short AT_UISTATISTICS = 16;

	enum ALARMLEVEL		//报警级别
	{
		//AL_OK = 0x0,
		AL_1 = 0x01,
		AL_2,
		AL_3,

		AL_4,//统计
	};
#pragma pack(1)
	struct JsonHeader	//报警和监控的Json协议头,报警和监控的数据通讯采用json协议,协议头采用二进制
	{
		uint32_t m_len;	//长度
		uint16_t m_cmd;	//协议命令字
		uint32_t m_seq;	//协议序列号
	};
#pragma pack()
	struct AlarmInfo
	{
		int level;	//报警级别
		short type;	//报警类型
		//char info[128];
		string info;	//报警内容
	};
	class Alarm
	{
		public:
			Alarm(FDEventHandler *handler) : m_handler(handler){}
			~Alarm(){
				close(m_fd[0]);
				close(m_fd[1]);
			}
			bool Init();
			/*inline void Alarm0(short type,const char *info){
				Notify(AL_OK,type,info);
			}*/
			inline void Alarm1(short type,const char *info){
				Notify(AL_1,type,info);
			}
			inline void Alarm2(short type,const char *info){
				Notify(AL_2,type,info);
			}
			inline void Alarm3(short type,const char *info){
				Notify(AL_3,type,info);
			}
			inline void Alarm4(short type,const char *info){
				Notify(AL_4,type,info);
			}
			inline void InfoPush(const string & info){
				Notify(info);
			}
			SyncQueue<AlarmInfo> m_alarmqueue;		//报警信息入队
			SyncQueue<string> m_infoqueue;		
		private:
			void Notify(int level,short type,const char *info);	//通知报警线程
			void Notify(const string & info);	
			int m_fd[2];
			FDEventHandler *m_handler;
	};
	extern Alarm *g_alarm;
	void Alarm1(short type,const char *info);	//级别1报警
	void Alarm2(short type,const char *info);	//级别2报警
	void Alarm3(short type,const char *info);	//级别3报警
	void Alarm4(short type,const char *info);	//级别4报警
	void InfoPush(const string & info);	
}


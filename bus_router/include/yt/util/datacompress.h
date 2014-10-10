#pragma once

#include <stdlib.h>
#include <sys/types.h>
#include <string>
#include <vector>
#include <sstream>
#include <netinet/in.h>
#include "protocolstream.h"


using namespace std;

namespace yt
{

	#define RATIO_LEN 3
	#define ZlibTag 0x01
	#define NotZlibTag 0x00
        #define THRESHOLD 4096
	#define EOF_ -1
	#define MAXINTSPACE 18
	const int64_t base = 1;
	const int64_t MAXDOUBLEVALUE[18][2]={
		                             {(base<<63-1)/10,(-base<<63)/10},
		                             {(base<<63-1)/100,(-base<<63)/100},
					     {(base<<63-1)/1000,(-base<<63)/1000},
					     {(base<<63-1)/10000,(-base<<63)/10000},
					     {(base<<63-1)/100000,(-base<<63)/100000},
					     {(base<<63-1)/1000000,(-base<<63)/1000000},
					     {(base<<63-1)/10000000,(-base<<63)/10000000},
					     {(base<<63-1)/100000000,(-base<<63)/100000000},
					     {(base<<63-1)/1000000000,(-base<<63)/1000000000},
					     {(base<<63-1)/10000000000,(-base<<63)/10000000000},
					     {(base<<63-1)/100000000000,(-base<<63)/100000000000},
					     {(base<<63-1)/1000000000000,(-base<<63)/1000000000000},
					     {(base<<63-1)/10000000000000,(-base<<63)/10000000000000},
					     {(base<<63-1)/100000000000000,(-base<<63)/100000000000000},
					     {(base<<63-1)/1000000000000000,(-base<<63)/1000000000000000},
					     {(base<<63-1)/10000000000000000,(-base<<63)/10000000000000000},
					     {(base<<63-1)/100000000000000000,(-base<<63)/100000000000000000},
					     {(base<<63-1)/1000000000000000000,(-base<<63)/1000000000000000000}
     
	};

	enum  OPTFLAG
	{
		ADAPTIVE=0,
		COMPRESS,
		NOCOMPRESS
	};

	class CompressBinaryWriteStream : public WriteStreamImpl
	{
		public:
			CompressBinaryWriteStream(string* data);
			bool Write(char c,int pos = EOF_);
			bool WriteNULL(int pos=EOF_);
			bool Write(short i,int pos = EOF_);
			bool Write(unsigned short i,int pos = EOF_);
			bool Write(int i,int pos = EOF_);
			bool Write(unsigned int i,int pos = EOF_);
			bool Write(int64_t i,int pos = EOF_);
			bool Write(uint64_t i,int pos = EOF_);
			bool Write(double i,int pos = EOF_);
			bool Write(const char* str,size_t len,int pos= EOF_);
			bool WriteDouble(const char* str,int pos= EOF_);
			void Flush(OPTFLAG opt=ADAPTIVE);
			size_t GetSize() const;
			const char* GetData() const;
			virtual ~CompressBinaryWriteStream();
			void Clear();
			size_t GetCurPos();
		private:
			bool ZlibCompress();
			bool Compress(int64_t i, char *buf, size_t &len);
			bool U_Compress(uint64_t i, char *buf, size_t &len);
			string * m_data;
			
	};
	class CompressBinaryReadStream : public ReadStreamImpl
	{
		public:
			CompressBinaryReadStream(const char* ptr_,size_t len_);
			bool Read(char &c);
			bool IsNULL();
			bool Read(short &i);
			bool Read(unsigned short&i);
			bool Read(int &i);
			bool Read(unsigned int &i);
			bool Read(int64_t &i);
			bool Read(uint64_t &i);
			bool Read(double &d);
			bool ReadDouble(string &sd);
			bool Read(char *str,size_t maxlen, size_t &outlen);
			bool Read(const char** str,size_t maxlen,size_t& outlen);
			bool Read(string *str,size_t maxlen,size_t& outlen);
			const char* GetData() const;
	                size_t GetSize() const;
			virtual ~CompressBinaryReadStream();

		private:
			bool isZlibCompress();
			bool ZlibUnCompress();
			bool UnCompress(int64_t &i,const char *,size_t &len);
			bool U_UnCompress(uint64_t &i,const char *,size_t &len);
			const char* ptr;
			const char* cur;
			size_t length;
			char stackdest[65535];		
			char* dest;
	};
}


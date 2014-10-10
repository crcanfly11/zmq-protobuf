#pragma once

#include <stdlib.h>
#include <sys/types.h>
#include <string>
#include <vector>
#include <sstream>
#include <netinet/in.h>
#include "yt/util/protocolstream.h"



using namespace std;

namespace yt
{


	#define EOF_ -1


	class CellBinaryWriteStream : public WriteStreamImpl
	{
		public:
			CellBinaryWriteStream(string* data);
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
			void Flush();
			size_t GetSize() const;
			const char* GetData() const;
			virtual ~CellBinaryWriteStream();
			void Clear();
			size_t GetCurPos();
		private:
			bool Compress(int64_t i, char *buf, size_t &len);
			bool U_Compress(uint64_t i, char *buf, size_t &len);
			string * m_data;
			
	};
	class CellBinaryReadStream : public ReadStreamImpl
	{
		public:
			CellBinaryReadStream(const char* ptr_,size_t len_);
			bool Read(char &c);
			bool IsNULL();
			bool Read(short &i);
			bool Read(unsigned short&i);
			bool Read(int &i);
			bool Read(unsigned int &i);
			bool Read(int64_t &i);
			bool Read(uint64_t &i);
			bool Read(double &d);
			bool Read(char *str,size_t maxlen, size_t &outlen);
			bool Read(const char** str,size_t maxlen,size_t& outlen);
			bool Read(string *str,size_t maxlen,size_t& outlen);
			const char* GetData() const;
	                size_t GetSize() const;
			virtual ~CellBinaryReadStream();

		private:
			bool UnCompress(int64_t &i,const char *,size_t &len);
			bool U_UnCompress(uint64_t &i,const char *,size_t &len);
			const char* ptr;
			const char* cur;
			size_t length;
	};
}


#pragma once

#include <stdlib.h>
#include <sys/types.h>
#include <string>
#include <sstream>
#include <netinet/in.h>

using namespace std;
//二进制协议的打包解包类，内部的服务器之间通讯，统一采用这些类
namespace yt
{
	enum {
		TEXT_PACKLEN_LEN	= 4,
		TEXT_PACKAGE_MAXLEN	= 0xffff,
		BINARY_PACKLEN_LEN	= 2,
		BINARY_PACKAGE_MAXLEN	= 0xffff,

		TEXT_PACKLEN_LEN_2	= 6,
		TEXT_PACKAGE_MAXLEN_2	= 0xffffff,

		BINARY_PACKLEN_LEN_2 	= 4,//4字节头长度
		BINARY_PACKAGE_MAXLEN_2 = 0x10000000,//包最大长度是256M,足够了

		CHECKSUM_LEN		= 2,
	};

	//计算校验和
	unsigned short checksum(const unsigned short *buffer,int size);
	bool compress_(uint32_t i, char *buf, size_t &len);
	bool uncompress_(char *buf, size_t len, uint32_t &i);

	struct ReadStreamImpl
	{
		virtual ~ReadStreamImpl() {}
		virtual const char* GetData() const = 0;
		virtual size_t GetSize() const = 0;
	};

	struct WriteStreamImpl
	{
		virtual ~WriteStreamImpl() {}
		virtual const char* GetData() const = 0;
		virtual size_t GetSize() const = 0;
	};

	/*class BinaryReadStream : public ReadStreamImpl
	{
		private:
			const char* const ptr;
			const size_t len;
			const char* cur;
			BinaryReadStream(const BinaryReadStream&);
			BinaryReadStream& operator=(const BinaryReadStream&);
		public:
			BinaryReadStream(const char* ptr, size_t len);
			//bool CheckSum();
			virtual size_t GetSize() const;
			virtual bool IsEmpty() const;
			virtual bool Read(string *str, size_t& outlen);
			virtual bool Read(char* str, size_t strlen, size_t& len);
			virtual bool Read( int & i);
			virtual bool Read( short & i);
			virtual bool Read( char & c);
			virtual const char* GetData() const;
			virtual size_t ReadAll(char * szBuffer, size_t iLen) const;
			virtual bool IsEnd() const;
		public:
			bool ReadLength(size_t & len);
			bool ReadLengthWithoutOffset(size_t & outlen);	
	};

	class BinaryWriteStream : public WriteStreamImpl
	{
		private:
			char* const ptr;
			const size_t len;
			char*  cur;
			BinaryWriteStream(const BinaryWriteStream&);
			BinaryWriteStream& operator=(const BinaryWriteStream&);
		public:
			BinaryWriteStream(char* ptr, size_t len);
			virtual bool Write(const char* str, size_t len);
			virtual bool Write(int i);
			virtual bool Write(short i);
			virtual bool Write(char c);	
			virtual size_t GetSize() const;
			virtual bool IsValid() const;
			//void CheckSum();
			virtual void Flush();
			virtual void Clear();
			virtual const char* GetData() const;
			virtual char* GetCurrent() const
			{ return cur; }
		public:
			bool WriteLength(size_t len);
	};*/

	class BinaryReadStream2 : public ReadStreamImpl
	{
		private:
			const char* const ptr;
			const size_t len;
			const char* cur;
			BinaryReadStream2(const BinaryReadStream2&);
			BinaryReadStream2& operator=(const BinaryReadStream2&);
		public:
			BinaryReadStream2(const char* ptr, size_t len);
			//bool CheckSum();
			virtual const char* GetData() const;
			virtual size_t GetSize() const;
			bool IsEmpty() const;
			bool Read(string *str,size_t maxlen,size_t& outlen);
			bool Read(char* str,size_t strlen,size_t& len);
			bool Read(const char** str,size_t maxlen,size_t& outlen);
			bool Read(int &i);
			bool Read(short &i);
			bool Read(char &c);
			size_t ReadAll(char * szBuffer, size_t iLen) const;
			bool IsEnd() const;
			const char* GetCurrent() const{ return cur; }

		public:
			bool ReadLength(size_t & len);
			bool ReadLengthWithoutOffset(size_t &headlen, size_t & outlen);
	};
	class BinaryWriteStream2 : public WriteStreamImpl
	{
		protected:
			char* const ptr;
			const size_t len;
			char*  cur;
			BinaryWriteStream2(const BinaryWriteStream2&);
			BinaryWriteStream2& operator=(const BinaryWriteStream2&);
		public:
			BinaryWriteStream2(char* ptr, size_t len);
			virtual const char* GetData() const;
			virtual size_t GetSize() const;
			bool Write(const char* str, size_t len);
			bool Write(int i);
			bool Write(short i);
			bool Write(char c);
			bool IsValid() const;
			//void CheckSum();
			void Flush();
			void Clear();				
			char* GetCurrent() const{ return cur; }
			bool Skip(size_t len_);
		public:
			bool WriteLength(size_t len);
	};
	class BinaryWriteStream3 : public WriteStreamImpl
	{
		public:
			BinaryWriteStream3(string *data);
			virtual const char* GetData() const;
			virtual size_t GetSize() const;
			bool Write(const char* str,size_t len);
			bool Write(int i);
			bool Write(short i);
			bool Write(char c);
			//void CheckSum(char *ptr);
			size_t GetCurrentPos() const{return m_data->length();}
			void Flush();
			void Clear();
		private:
			string *m_data;
	};
}



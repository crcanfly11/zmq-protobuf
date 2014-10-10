#include "protocolstream.h"
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <cassert>
#include <algorithm>

using namespace std;

namespace yt 
{
	//计算校验和
	unsigned short checksum(const unsigned short *buffer,int size)
	{
		unsigned int cksum = 0;
		while(size > 1)
		{
			cksum += *buffer++;
			size -= sizeof(unsigned short);
		}
		if(size)
		{
			cksum += *(unsigned char*)buffer;
		}
		//将32位数转换成16
		while (cksum >> 16)
			cksum = (cksum >> 16) + (cksum & 0xffff);
		return (unsigned short)(~cksum);
	}

	bool compress_(uint32_t i, char *buf, size_t &len)
	{
		len = 0;
		for (int a=4; a>=0; a--)
		{
			char c;
			c = i >> (a*7) & 0x7f;
			if (c == 0x00 && len == 0)
				continue;

			if (a == 0)
				c &= 0x7f;
			else
				c |= 0x80;
			buf[len] = c;
			len++;
		}
		if (len == 0)
		{
			len++;
			buf[0] = 0;
		}

		//cout << "compress:" << i << endl;
		//cout << "compress len:" << len << endl;
		return true;
	}

	bool uncompress_(char *buf, size_t len, uint32_t &i)
	{
		i = 0;
		for (int index = 0; index < (int)len; index++)
		{
			char c = *(buf + index);
			i = i << 7;

			c &= 0x7f;
			i |= c;
		}
		//cout << "uncompress:" << i << endl;
		return true;
	}

	/*BinaryReadStream::BinaryReadStream(const char* ptr_, size_t len_)
		: ptr(ptr_), len(len_), cur(ptr_)
		{
			cur += BINARY_PACKLEN_LEN + CHECKSUM_LEN;
		}
	bool BinaryReadStream::IsEmpty() const
	{
		return len<=BINARY_PACKLEN_LEN;
	}
	bool BinaryReadStream::CheckSum()
	{
		const unsigned short cs = checksum(reinterpret_cast<const unsigned short*>(ptr + BINARY_PACKLEN_LEN + CHECKSUM_LEN),len - BINARY_PACKLEN_LEN - CHECKSUM_LEN);
		const unsigned short *pcs = reinterpret_cast<const unsigned short*>(ptr + BINARY_PACKLEN_LEN);
		
		if(cs == ntohs(*pcs))
			return true;		

		return false;
	}
	size_t BinaryReadStream::GetSize() const
	{
		return len;
	}
	bool BinaryReadStream::Read(char* str, size_t strlen,size_t& outlen)
	{
		size_t fieldlen;
		if ( !ReadLengthWithoutOffset(fieldlen) ) {
			return false;
		}

		// user buffer is not enough
		if ( strlen < fieldlen ) {
			return false;
		}

		// 偏移到数据的位置
		cur += BINARY_PACKLEN_LEN;	

		memcpy(str, cur, fieldlen);
		outlen = fieldlen;

		cur += outlen;

		return true;
	}
	bool BinaryReadStream::Read(string *str,size_t& outlen)
	{
		size_t fieldlen;
		if ( !ReadLengthWithoutOffset(fieldlen) ) {
			return false;
		}

		// 偏移到数据的位置
		cur += BINARY_PACKLEN_LEN;

		str->assign(cur,fieldlen);
		outlen = fieldlen;

		cur += outlen;

		return true;
	}
	bool BinaryReadStream::Read(int & i)
	{
		const int VALUE_SIZE = sizeof(int);

		if ( cur + VALUE_SIZE > ptr + len ) {
			return false;
		}

		memcpy(&i, cur, VALUE_SIZE);
		i = ntohl(i);

		cur += VALUE_SIZE;

		return true;
	}
	bool BinaryReadStream::Read(short & i)
	{
		const int VALUE_SIZE = sizeof(short);

		if ( cur + VALUE_SIZE > ptr + len ) {
			return false;
		}

		memcpy(&i, cur, VALUE_SIZE);
		i = ntohs(i);

		cur += VALUE_SIZE;

		return true;
	}
	bool BinaryReadStream::Read(char & c)
	{
		const int VALUE_SIZE = sizeof(char);

		if ( cur + VALUE_SIZE > ptr + len ) {
			return false;
		}

		memcpy(&c, cur, VALUE_SIZE);
		cur += VALUE_SIZE;

		return true;
	}
	bool BinaryReadStream::ReadLength(size_t & outlen)
	{
		if ( !ReadLengthWithoutOffset(outlen) ) {
			return false;
		}

		cur += BINARY_PACKLEN_LEN;
		return true;
	}
	bool BinaryReadStream::ReadLengthWithoutOffset(size_t & outlen)
	{
		if ( cur + BINARY_PACKLEN_LEN > ptr + len ) {
			return false;
		}

		unsigned short tmp;
		memcpy(&tmp, cur, sizeof(tmp));
		outlen = ntohs(tmp);
		return true;
	}
	bool BinaryReadStream::IsEnd() const
	{
		assert(cur <= ptr + len);
		return cur == ptr + len;
	}
	const char* BinaryReadStream::GetData() const
	{
		return ptr;
	}
	size_t BinaryReadStream::ReadAll(char * szBuffer, size_t iLen) const
	{
		size_t iRealLen = min(iLen, len);
		memcpy(szBuffer, ptr, iRealLen);
		return iRealLen;
	}
	BinaryWriteStream::BinaryWriteStream(char* ptr_, size_t len_)
		: ptr(ptr_), len(len_), cur(ptr_)
		{
			cur += BINARY_PACKLEN_LEN + CHECKSUM_LEN;
		}
	bool BinaryWriteStream::Write(const char* str, size_t length)
	{
		if ( cur + length + BINARY_PACKLEN_LEN > ptr + len ) {
			return false;
		}

		if ( !WriteLength(length) ) {
			cur -= BINARY_PACKLEN_LEN;
			return false;
		}

		memcpy(cur, str, length);
		cur += length;

		return true;
	}
	bool BinaryWriteStream::Write(int i)
	{
		if (cur + sizeof(i) > ptr + len) {
			return false;
		}

		int iTmp = htonl(i);
		memcpy(cur, &iTmp, sizeof(iTmp));
		cur += sizeof(iTmp);

		return true;
	}
	bool BinaryWriteStream::Write(short i)
	{
		if (cur + sizeof(i) > ptr + len) {
			return false;
		}

		short iTmp = htons(i);
		memcpy(cur, &iTmp, sizeof(iTmp));
		cur += sizeof(iTmp);

		return true;
	}
	bool BinaryWriteStream::Write(char c)
	{
		if (cur + sizeof(c) > ptr + len) {
			return false;
		}

		*cur = c;
		++cur;
		return true;
	}
	bool BinaryWriteStream::WriteLength(size_t length)
	{
		unsigned short ulen = length;

		ulen = htons(ulen);
		memcpy(cur, &ulen, sizeof(ulen));
		cur += sizeof(ulen);
		return true;
	}
	size_t BinaryWriteStream::GetSize() const
	{
		return cur - ptr;
	}
	bool BinaryWriteStream::IsValid() const
	{
		size_t datalen = GetSize();
		return datalen > BINARY_PACKLEN_LEN && datalen <= BINARY_PACKAGE_MAXLEN;
	}
	void BinaryWriteStream::Flush()
	{
		unsigned short datalen = GetSize();
		unsigned short* ps = reinterpret_cast<unsigned short*>(ptr);
		*ps = htons(datalen);

		//CheckSum();
	}
	void BinaryWriteStream::CheckSum()
	{
		unsigned short datalen = GetSize();
		unsigned short* cs = reinterpret_cast<unsigned short*>(ptr + BINARY_PACKLEN_LEN);
		unsigned short cs2 = htons(checksum(reinterpret_cast<unsigned short*>(ptr + BINARY_PACKLEN_LEN + CHECKSUM_LEN),datalen - BINARY_PACKLEN_LEN - CHECKSUM_LEN));
		*cs = cs2;
	}
	void BinaryWriteStream::Clear()
	{
		cur = ptr + BINARY_PACKLEN_LEN + CHECKSUM_LEN;
	}
	const char* BinaryWriteStream::GetData() const
	{
		return ptr;
	}
	bool BinaryReadStream2::CheckSum()
	{
		const unsigned short cs = checksum(reinterpret_cast<const unsigned short*>(ptr + BINARY_PACKLEN_LEN_2 + CHECKSUM_LEN),len - BINARY_PACKLEN_LEN_2 - CHECKSUM_LEN);
		const unsigned short *pcs = reinterpret_cast<const unsigned short*>(ptr + BINARY_PACKLEN_LEN_2);
		
		if(cs == ntohs(*pcs))
			return true;		

		return false;
	}
*/
	BinaryReadStream2::BinaryReadStream2(const char* ptr_, size_t len_)
		: ptr(ptr_), len(len_), cur(ptr_)
		{
			cur += BINARY_PACKLEN_LEN_2 + CHECKSUM_LEN;
		}
	bool BinaryReadStream2::IsEmpty() const
	{
		return len<=BINARY_PACKLEN_LEN_2;
	}
	size_t BinaryReadStream2::GetSize() const
	{
		return len;
	}
	bool BinaryReadStream2::Read(char* str, size_t strlen, /* out */ size_t& outlen)
	{
		size_t fieldlen;
		size_t headlen;
		if ( !ReadLengthWithoutOffset(headlen, fieldlen) ) {
			return false;
		}

		// user buffer is not enough
		if (fieldlen > strlen) {
			return false;
		}

		// 偏移到数据的位置
		//cur += BINARY_PACKLEN_LEN_2;	
		cur += headlen;	
    if(cur + fieldlen > ptr+len)
		{
			outlen=0;
			return false;
		}
		memcpy(str, cur, fieldlen);
		outlen = fieldlen;  
		cur += outlen;    
		return true;
	}
	bool BinaryReadStream2::Read(string *str,size_t maxlen,size_t& outlen)
	{
		size_t headlen;
		size_t fieldlen;
		if ( !ReadLengthWithoutOffset(headlen, fieldlen) ) {
			return false;
		}

		// user buffer is not enough
		if (maxlen != 0 && fieldlen > maxlen) {
			return false;
		}

		// 偏移到数据的位置
		//cur += BINARY_PACKLEN_LEN_2;	
		cur += headlen;	
    if(cur + fieldlen > ptr+len)
		{
			outlen=0;
			return false;
		}
		str->assign(cur,fieldlen);
		outlen = fieldlen;   
		cur += outlen;		
		return true;
	}
	bool BinaryReadStream2::Read(const char** str,size_t maxlen,size_t& outlen)
	{
		size_t headlen;
		size_t fieldlen;
		if ( !ReadLengthWithoutOffset(headlen, fieldlen) ) {
			return false;
		}
		// user buffer is not enough
		if (maxlen != 0 && fieldlen > maxlen) {
			return false;
		}

		// 偏移到数据的位置
		//cur += BINARY_PACKLEN_LEN_2;	
		cur += headlen;

		//memcpy(str, cur, fieldlen);
		if(cur + fieldlen > ptr+len)
		{
			outlen=0;
			return false;
		}
		*str = cur;
		outlen = fieldlen; 
		cur += outlen;
		return true;
	}
	bool BinaryReadStream2::Read(int &i)
	{
		const int VALUE_SIZE = sizeof(int);

		if ( cur + VALUE_SIZE > ptr + len ) {
			return false;
		}

		memcpy(&i, cur, VALUE_SIZE);
		i = ntohl(i);

		cur += VALUE_SIZE;

		return true;
	}
	bool BinaryReadStream2::Read(short &i)
	{
		const int VALUE_SIZE = sizeof(short);

		if ( cur + VALUE_SIZE > ptr + len ) {
			return false;
		}

		memcpy(&i, cur, VALUE_SIZE);
		i = ntohs(i);

		cur += VALUE_SIZE;

		return true;
	}
	bool BinaryReadStream2::Read(char &c)
	{
		const int VALUE_SIZE = sizeof(char);

		if ( cur + VALUE_SIZE > ptr + len ) {
			return false;
		}

		memcpy(&c, cur, VALUE_SIZE);
		cur += VALUE_SIZE;

		return true;
	}
	bool BinaryReadStream2::ReadLength(size_t & outlen)
	{
		size_t headlen;
		if ( !ReadLengthWithoutOffset(headlen, outlen) ) {
			return false;
		}

		//cur += BINARY_PACKLEN_LEN_2;
		cur += headlen;
		return true;
	}
	bool BinaryReadStream2::ReadLengthWithoutOffset(size_t &headlen, size_t & outlen)
	{
		headlen = 0;
		const char *temp = cur;
		char buf[5];
		for (size_t i=0; i<sizeof(buf); i++)
		{
			memcpy(buf + i, temp, sizeof(char));
			temp++;
			headlen++;

			//if ((buf[i] >> 7 | 0x0) == 0x0)
			if ((buf[i] & 0x80) == 0x00)
				break;
		}
		if (cur + headlen > ptr +len)
			return false;
	
		uint32_t value; 	
		uncompress_(buf, headlen, value);
		outlen = value;

		/*if ( cur + BINARY_PACKLEN_LEN_2 > ptr + len ) {
			return false;
		}

		unsigned int tmp;
		memcpy(&tmp, cur, sizeof(tmp));
		outlen = ntohl(tmp);*/
		return true;
	}
	bool BinaryReadStream2::IsEnd() const
	{
		assert(cur <= ptr + len);
		return cur == ptr + len;
	}
	const char* BinaryReadStream2::GetData() const
	{
		return ptr;
	}
	size_t BinaryReadStream2::ReadAll(char * szBuffer, size_t iLen) const
	{
		size_t iRealLen = min(iLen, len);
		memcpy(szBuffer, ptr, iRealLen);
		return iRealLen;
	}
	BinaryWriteStream2::BinaryWriteStream2(char* ptr_, size_t len_)
		: ptr(ptr_), len(len_), cur(ptr_)
		{
			cur += BINARY_PACKLEN_LEN_2 + CHECKSUM_LEN;
		}
	bool BinaryWriteStream2::Write(const char* str, size_t length)
	{
		/*if ( cur + length + BINARY_PACKLEN_LEN_2 > ptr + len ) {
			return false;
		}
		if ( !WriteLength(length) ) {
			cur -= BINARY_PACKLEN_LEN_2;
			return false;
		}*/
		if (!WriteLength(length))
		{
			return false;
		}

		memcpy(cur, str, length);
		cur += length;

		return true;
	}
	bool BinaryWriteStream2::Write(int i)
	{
		if (cur + sizeof(i) > ptr + len) {
			return false;
		}

		int iTmp = htonl(i);
		memcpy(cur, &iTmp, sizeof(iTmp));
		cur += sizeof(iTmp);

		return true;
	}
	bool BinaryWriteStream2::Write(short i)
	{
		if (cur + sizeof(i) > ptr + len) {
			return false;
		}

		short iTmp = htons(i);
		memcpy(cur, &iTmp, sizeof(iTmp));
		cur += sizeof(iTmp);

		return true;
	}
	bool BinaryWriteStream2::Write(char c)
	{
		if (cur + sizeof(c) > ptr + len) {
			return false;
		}

		*cur = c;
		++cur;
		return true;
	}
	bool BinaryWriteStream2::WriteLength(size_t length)
	{
		char buf[5];
		size_t buflen;
		compress_(length, buf, buflen);
		memcpy(cur, buf, sizeof(char)*buflen);
		cur += buflen*sizeof(char);

		if ( cur + length > ptr + len ) {
			cur -= buflen;
			return false;
		}

		/*unsigned int ulen = length;
		ulen = htonl(ulen);
		memcpy(cur, &ulen, sizeof(ulen));
		cur += sizeof(ulen);*/
		return true;
	}
	size_t BinaryWriteStream2::GetSize() const
	{
		return cur - ptr;
	}
	bool BinaryWriteStream2::IsValid() const
	{
		size_t datalen = GetSize();
		return datalen > BINARY_PACKLEN_LEN_2 && datalen <= BINARY_PACKAGE_MAXLEN_2;
	}
	void BinaryWriteStream2::Flush()
	{
		unsigned int datalen = GetSize();
		unsigned int* ps = reinterpret_cast<unsigned int*>(ptr);
		*ps = htonl(datalen);

		//CheckSum();
	}
	/*void BinaryWriteStream2::CheckSum()
	{
		unsigned int datalen = GetSize();
		unsigned short* cs = reinterpret_cast<unsigned short*>(ptr + BINARY_PACKLEN_LEN_2);
		unsigned short cs2 = htons(checksum(reinterpret_cast<unsigned short*>(ptr + BINARY_PACKLEN_LEN_2 + CHECKSUM_LEN),datalen - BINARY_PACKLEN_LEN_2 - CHECKSUM_LEN));
		*cs = cs2;
	}*/
	void BinaryWriteStream2::Clear()
	{
		cur = ptr + BINARY_PACKLEN_LEN_2 + CHECKSUM_LEN;
	}
	const char* BinaryWriteStream2::GetData() const
	{
		return ptr;
	}
	bool BinaryWriteStream2::Skip(size_t len_)
	{
		if(cur + len_ > ptr + len) 
			return false;
		cur += len_;
		return true;
	}
	BinaryWriteStream3::BinaryWriteStream3(string *data) : 
		m_data(data)
	{
		m_data->clear();
		char str[BINARY_PACKLEN_LEN_2 + CHECKSUM_LEN];
		m_data->append(str,sizeof(str));
	}
	bool BinaryWriteStream3::Write(const char* str,size_t len)
	{
		char buf[5];
		size_t buflen;
		compress_(len, buf, buflen);
		m_data->append(buf, sizeof(char)*buflen);

		m_data->append(str, len);

		//unsigned int ulen = htonl(len);
		//m_data->append((char*)&ulen,sizeof(ulen));
		//m_data->append(str,len);
		return true;
	}
	const char* BinaryWriteStream3::GetData() const
	{
		return m_data->data();
	}
	size_t BinaryWriteStream3::GetSize() const
	{
		return m_data->length();
	}
	bool BinaryWriteStream3::Write(int i)
	{
		int i2 = htonl(i);
		m_data->append((char*)&i2,sizeof(i2));
		return true;
	}
	bool BinaryWriteStream3::Write(short i)
	{
		short i2 = htons(i);
		m_data->append((char*)&i2,sizeof(i2));
		return true;
	}
	bool BinaryWriteStream3::Write(char c)
	{
		//m_data->append(&c,1);
		(*m_data) += c;
		return true;
	}
	/*void BinaryWriteStream3::CheckSum(char *ptr)
	{
		unsigned int datalen = GetSize();
		unsigned short* cs = reinterpret_cast<unsigned short*>(ptr + BINARY_PACKLEN_LEN_2);
		unsigned short cs2 = htons(checksum(reinterpret_cast<unsigned short*>(ptr + BINARY_PACKLEN_LEN_2 + CHECKSUM_LEN),datalen - BINARY_PACKLEN_LEN_2 - CHECKSUM_LEN));
		*cs = cs2;
	}*/
	void BinaryWriteStream3::Flush()
	{
		char *ptr = &(*m_data)[0];
		//CheckSum(ptr);
		unsigned int ulen = htonl(m_data->length());
		memcpy(ptr,&ulen,sizeof(ulen));
	}
	void BinaryWriteStream3::Clear()
	{
		m_data->clear();
		char str[BINARY_PACKLEN_LEN_2 + CHECKSUM_LEN];
		m_data->append(str,sizeof(str));
	}
}


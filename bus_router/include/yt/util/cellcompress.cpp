#include "cellcompress.h"
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <cassert>
#include <algorithm>
#include<vector>
#include<math.h>
#include<string>
#include<iostream>

using namespace std;

namespace yt 
{


	CellBinaryWriteStream::CellBinaryWriteStream(string *data) : m_data(data)
	{
		m_data->clear();
	}

	bool CellBinaryWriteStream::Compress(int64_t i, char *buf, size_t &len)
		{
			//负数变正数
			if(i < 0)
			{
				i = -i;
				buf[0] = 0x80;
			}
			else
			{
				buf[0] = 0x00;
			}
			len = 0;
			//找出第一个不为0的位数
			int index = 0;
			while(index!=64)
			{
				if(i == 1)
				{
					index = 64 - (index+1);
					break;
				}
				if((i >> (64 - (index+1)))!=0)
				{
					break;
				}
				index++;
			}
			int mul = (64 - index) / 7; 
			int rem = (64 - index) % 7;
			int64_t temp = (int64_t)(pow(2,rem)) - 1;
			char remvalue =  (char)(((temp << (mul*7))  & i) >> (7*mul));
			buf[0] = buf[0]|remvalue;
			if(mul > 0)
			{
				buf[0] = buf[0]|0x40;
			}
			int64_t itmp=(int64_t)(pow(2,mul*7)) - 1;
			len=mul + 1;
			if(len == 1)
				return true;
			i=(itmp&i)<<(64-mul*7);
			index = len - 1;
			for(int a = mul;a >= 1;a--)
			{
				char mulvalue = 0;
				mulvalue = ((i >> (64 -a*7) )); 
				buf[index] = mulvalue & 0x7f;
				if(a == mul)
				{
					buf[index]=buf[index]&0x7f;
				}
				else
					buf[index] = buf[index]|0x80;
				int temp = 0;
				temp = buf[index];
				index--;
			}
			return true;
		}


	bool CellBinaryWriteStream::U_Compress(uint64_t i, char *buf, size_t &len)
	{
		len = 0;
		for (int a=9; a>=0; a--)
		{
			char c;
			c = i >> (a*7) & 0x7f;
			if (c == 0x00 && len == 0)
			{
				continue;
			}

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
		return true;
	}



	bool CellBinaryWriteStream::Write(char c,int pos)
	{    
		if(pos==EOF_)
		     m_data->append((char*)&c,sizeof(char));
		else
		     m_data->insert(pos,sizeof(char),c);
		return true;
	}

	bool CellBinaryWriteStream::WriteNULL(int pos)
	{
		char buf[1];
		buf[0]=0x80;
		if(pos==EOF_)
			m_data->append(buf,sizeof(buf));
		else
			m_data->insert(pos,buf,1);
		return true;
	}


	bool CellBinaryWriteStream::Write(short i,int pos)
	{
		Write((int64_t)i,pos);
		return true;
	}
       
       bool CellBinaryWriteStream::Write(unsigned short i,int pos)
       {
	       Write((uint64_t)i,pos);
	       return true;
       }
 

	bool CellBinaryWriteStream::Write(int i,int pos)
	{
		Write((int64_t)i,pos);
		return true;
	}

	bool CellBinaryWriteStream::Write(unsigned int i,int pos)
	{  
		Write((uint64_t)i,pos);
		return true;

	}
	bool CellBinaryWriteStream::Write(int64_t i,int pos)
	{   
		char buf[11];
		size_t len;
		Compress(i,buf,len);
		if(pos==EOF_)
	            m_data->append(buf,len);
		else
		    m_data->insert(pos,buf,len);
		return true;
	}

	bool CellBinaryWriteStream::Write(uint64_t i,int pos)
	{
		char buf[11];
		size_t len;
		U_Compress(i,buf,len);
		if(pos==EOF_)
			 m_data->append(buf,len);
		else
			m_data->insert(pos,buf,len);
		return true;
	}


	bool CellBinaryWriteStream::Write(double i,int pos)
	{
		char str[128];
		sprintf(str,"%.7f",i);
		char * floatpos=strchr(str,'.');
		if(!floatpos)
		{
			return false;
		}
		int point=floatpos-str+1;
		int64_t intpart=(int64_t)atol(str);
		int64_t index=(int64_t)(strlen(str)-point);
		int64_t floatpart=strtol(floatpos+1,NULL,10);
		while(floatpart%10==0&&floatpart!=0)
		{
			floatpart/=10;
			index--;
		}
		int64_t num=0;
		int64_t temp;
		if(i>=0)
		{
			num=(int64_t)(intpart*pow(10,index))+floatpart;
			temp=num;
		}
		else
		{
			num=(int64_t)(intpart*pow(10,index))-floatpart;
			temp=-num;
		}
		//基数与指数合并
		temp=temp<<3;
		temp|=index;
		
		if(i<0)
			temp=-temp;
                Write(temp,pos);
		return true;
	}

	bool CellBinaryWriteStream::Write(const char* str, size_t len,int pos)
	{
		char buf[11];
		size_t outlen;
		U_Compress(len,buf,outlen);
		if(pos==EOF_)
		{
			m_data->append(buf,outlen);
			m_data->append(str,len);
		}
		else
		{
			m_data->insert(pos,buf,outlen);
			m_data->insert(pos+outlen,str);
		}
		return true;
	}
	
	size_t CellBinaryWriteStream::GetCurPos()
	{
	          return m_data->size();
	}

	size_t CellBinaryWriteStream::GetSize() const
	{
		return m_data->size();
	}

	const char* CellBinaryWriteStream::GetData() const
	{
		return m_data->data();
	}

	 void CellBinaryWriteStream::Clear()
	{
		m_data->clear();
	}

	 void CellBinaryWriteStream::Flush()
	 {
		 unsigned int ulen = m_data->length();
		 char buf[4];
		 size_t len;
		 U_Compress(ulen,buf,len);
		 m_data->insert(0,buf,len);
	 }

	 CellBinaryWriteStream:: ~CellBinaryWriteStream()
	 {

	 }

	CellBinaryReadStream::CellBinaryReadStream(const char* ptr_,size_t len_)
		: ptr(ptr_),cur(ptr_),length(len_) 
	{
		uint64_t i;
		const char * buf=ptr;
		size_t len;
		U_UnCompress(i,buf,len);
	        cur=ptr+len;
	}


	bool CellBinaryReadStream::Read(short &i)
	{
		int64_t i2=i;
		if(!Read(i2))
			return false;
		i = i2;
		return true;
	}

	bool CellBinaryReadStream::Read(unsigned short  &i)
	{
		uint64_t i2=i;
		if(!Read(i2))
			return false;
		i = i2;
		return true;
	}

	bool CellBinaryReadStream::Read(char &c)
	{
		const int VALUE_SIZE = sizeof(char);
		if ( cur + VALUE_SIZE > ptr + length )
		{
			return false;
		}
		memcpy(&c, cur, VALUE_SIZE);
		cur += VALUE_SIZE;
		return true;
	}

	bool CellBinaryReadStream::IsNULL()
	{
		if(cur-ptr==(int)length)
			return false;
		if((int)*cur != -128)
			return false;
		cur++;
		if(cur-ptr>(int)length)
		{
			cur--;
			return false;
		}
		return true;
	}


	bool CellBinaryReadStream::Read(int &i)
	{
		int64_t i2 = i;
		if(!Read(i2))
			return false;
		i = i2;
		return true;
	}

	bool CellBinaryReadStream::Read(unsigned int &i)
	{
		uint64_t i2=i;
		if(!Read(i2))
			return false;
		i = i2;
		return true;
	}

	bool CellBinaryReadStream::Read(int64_t &i)
	{ 
		if(cur-ptr==(int)length)
			return false;
		const char *buf=cur;
		size_t len;
		if(UnCompress(i,buf,len))
		{
			cur+=len;
			if(cur<=ptr+length)
			{
				return true;
			}
			else
			{
				cur-=len;
			}

		}
		return false;

	}

	bool CellBinaryReadStream::Read(uint64_t &i)
	{
		if(cur-ptr==(int)length)
			return false;
		const char *buf=cur;
		size_t len;
		if(U_UnCompress(i,buf,len))
		{
			cur+= len;
			if(cur<=ptr+length)
			{
				return true;
			}
			else
			{
				cur-=len;
			}

		}
		return false;

	}

	bool CellBinaryReadStream::UnCompress(int64_t &i,const char *buf,size_t &len)
	{
		char flag=(buf[0]&0x80)>>7;
		int j =1;
		if(flag==1)
			j=-1;
		int index=0;
		char memflag=(buf[0] & 0x40)>>6;
		if((int)memflag == 0)
		{
			buf+=index;
			len = 1;
			i=(buf[0]&0x3f);
		}	
		else
		{
			i=(buf[0]&0x3f);
			index += 1;
			while((*(buf+ index))>>7)
			{
				char c = *(buf+ index);
				i = i << 7;
				c &= 0x7f;
				i |= c;
				index++;
			}
			char c = *(buf+ index);
			i = i << 7;
			c &= 0x7f;
			i |= c;
			len=index+1;
		}
		i = j*i;
		return true;
	}


	bool CellBinaryReadStream::U_UnCompress(uint64_t &i,const char *buf,size_t &len)
	{
		i = 0;
		int index=0;
		while( (*(buf+ index))>>7)
		{
			char c = *(buf+ index);
			i = i << 7;
			c &= 0x7f;
			i |= c;
			index++;
		}
		char c = *(buf+ index);
		i = i << 7;
		c &= 0x7f;
		i |= c;
		len = index + 1;
		return true;

	}

	bool CellBinaryReadStream::Read(double &d)
	{
		int64_t i;
		if(!Read(i))
			return false;
		int64_t itmp=i;
		int64_t num=i;
		if(i<0)
		{
			itmp=-i;
			num=-i;
		}
		int64_t index=itmp&0x07;
		num=num>>3;
		if(i<0)
			num=-num;
		double p = pow(10,-index);
		d=num*p;
		return true;
	}

	bool CellBinaryReadStream:: Read(char *str,size_t maxlen, size_t &outlen)
	{
		uint64_t out;
		if(!Read(out))
		{
			outlen = 0;
			return false;
		}
		outlen=out;
		if (maxlen != 0 && outlen > maxlen) 
		{
			return false;
		}
		if(cur+outlen>ptr+length)
		{
			outlen=0;
			return false;
		}	
		memcpy(str, cur, outlen);
		cur+=outlen;
		return true;
	}


	bool CellBinaryReadStream:: Read(string *str,size_t maxlen,size_t& outlen)
	{
		uint64_t out;
		if(!Read(out))
		{
			outlen = 0;
			return false;
		}
		outlen=out;
		if (maxlen != 0 && outlen > maxlen) 
		{
			return false;
		}
		if(cur+outlen>ptr+length)
		{
			outlen=0;
			return false;
		}
		str->assign(cur, outlen);
		cur+=outlen;	
		return true;
	}

	bool CellBinaryReadStream::Read(const char** str,size_t maxlen,size_t& outlen)
	{
		uint64_t out;
		if(!Read(out))
		{
			outlen = 0;
			return false;
		}
		outlen=out;

		if (maxlen != 0 && outlen > maxlen) 
		{
			return false;
		}
		if(cur+outlen>ptr+length)
		{
			outlen=0;
			return false;
		}
		*str=cur;
		cur+=outlen;
		return true;

	}


	size_t CellBinaryReadStream::GetSize() const
	{
		return length;
	}

	const char* CellBinaryReadStream::GetData() const
	{
		return ptr;
	}

	CellBinaryReadStream:: ~CellBinaryReadStream()
	{

	}

}




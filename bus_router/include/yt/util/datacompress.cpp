#include "datacompress.h"
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <cassert>
#include <algorithm>
#include<vector>
#include<math.h>
#include <limits>
#include"zlib.h"
#include<string>
#include<iostream>

using namespace std;

namespace yt 
{

	std::string NumberToString(double d)
	{
		int prec = std::numeric_limits<double>::digits10;
		std::ostringstream os;
		//os.setf(ios::fixed); 
		os.precision(prec);
		os << d;
		return  os.str();
	}

	CompressBinaryWriteStream::CompressBinaryWriteStream(string *data) : m_data(data)
	{
		m_data->clear();
		char buf[7];
		m_data->append(buf,sizeof(buf));
	}

	bool CompressBinaryWriteStream::Compress(int64_t i, char *buf, size_t &len)
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


	bool CompressBinaryWriteStream::U_Compress(uint64_t i, char *buf, size_t &len)
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



	bool CompressBinaryWriteStream::Write(char c,int pos)
	{     
		if(pos==EOF_)
			m_data->append((char*)&c,sizeof(char));
		else
			m_data->insert(pos,sizeof(char),c);
		return true;
	}

	bool CompressBinaryWriteStream::WriteNULL(int pos)
	{
		char buf[1];
		buf[0]=0x80;
		if(pos==EOF_)
			m_data->append(buf,sizeof(buf));
		else
			m_data->insert(pos,buf,1);
		return true;
	}


	bool CompressBinaryWriteStream::Write(short i,int pos)
	{
		Write((int64_t)i,pos);
		return true;
	}

	bool CompressBinaryWriteStream::Write(unsigned short i,int pos)
	{
		Write((uint64_t)i,pos);
		return true;
	}

	bool CompressBinaryWriteStream::Write(int i,int pos)
	{
		Write((int64_t)i,pos);
		return true;
	}
	bool CompressBinaryWriteStream::Write(unsigned int i,int pos)
	{  
		Write((uint64_t)i,pos);
		return true;

	}
	bool CompressBinaryWriteStream::Write(int64_t i,int pos)
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

	bool CompressBinaryWriteStream::Write(uint64_t i,int pos)
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

	bool CompressBinaryWriteStream::Write(double i,int pos)
	{
		string str=NumberToString(i);
		bool isFindE = false;
		size_t findpos;
		findpos = str.find("e");
		if(findpos != string::npos)
			isFindE = true;
		if(!isFindE)
			return WriteDouble(str.c_str(),pos);
		else
		{
			char buf[256];
			sprintf(buf,"%.18f",i);
			return WriteDouble(buf,pos);
		}
	}

	bool CompressBinaryWriteStream::WriteDouble(const char* str,int pos)
	{
		if(strlen(str) == 0)
			return false;
		const char *floatpos=strchr(str,'.');
		char flag;
		//only int
		if(!floatpos)
		{
			int intlen = strlen(str);
			if(intlen > MAXINTSPACE)
			{
				if(pos==EOF_)
				{
					Write(str,intlen);
				}
				else
				{
					Write(str,intlen,pos);
				}
				return true;
			}
			int64_t  i=strtoll(str,NULL,10);
			flag=0xa0;
			if(pos==EOF_)
			{
				m_data->append(1,flag);
				Write(i);
			}
			else
			{
				m_data->insert(pos,1,flag);
				Write(i,pos+1);
			}
			return true;
		}
		int point=floatpos-str+1;
		string sint(str,point);
		sint[point - 1] = 0;
		int intlen = strlen(sint.c_str());
		//int bit >  int64
		if(intlen > MAXINTSPACE)
		{
			if(pos==EOF_)
			{
				Write(str,strlen(str));
			}
			else
			{
				Write(str,strlen(str),pos);
			}
			return true;
		}
		//int + float
		int64_t intpart = (int64_t)strtoll(sint.c_str(),NULL,10);
		int64_t index=(int64_t)(strlen(str)-point);
		int64_t floatpart=strtol(floatpos+1,NULL,10);
		if(floatpart==0)
		{
			if(intlen > MAXINTSPACE)
			{
				if(pos==EOF_)
				{
					Write(sint.c_str(),intlen);
				}
				else
				{
					Write(sint.c_str(),intlen,pos);
				}
				return true;
			}
			int64_t  i=strtoll(sint.c_str(),NULL,10);
			flag=0xa0;
			if(pos==EOF_)
			{
				m_data->append(1,flag);
				Write(i);
			}
			else
			{
				m_data->insert(pos,1,flag);
				Write(i,pos+1);
			}
			return true;
		}
		while(floatpart%10==0&&floatpart!=0)
		{
			floatpart/=10;
			index--;
		}
		if(index >18)
		{
			int diff=index -18;
			index =18;
			floatpart /= (diff * 10);
		}
		int64_t num=0;
		if(sint[0]!='-')
		{
			num=(int64_t)(intpart*(int64_t)(pow(10,index)))+floatpart;
		}
		else
		{
			num=(int64_t)(intpart*(int64_t)pow(10,index))-floatpart;
		}
		if(intpart > MAXDOUBLEVALUE[index -1][0] || intpart < MAXDOUBLEVALUE[index -1][1] )
		{
			if(pos==EOF_)
			{
				Write(str,strlen(str));
			}
			else
			{
				Write(str,strlen(str),pos);
			}
			return true;
		}
		flag=0xa0;
		flag|=(char)index;
		if(pos==EOF_)		
		{			
			m_data->append(1,flag);			
			Write(num);		
		}		
		else 		
		{			
			m_data->insert(pos,1,flag);			
			Write(num,pos+1);		
		}
		return true;
	}

	bool CompressBinaryWriteStream::Write(const char* str, size_t len,int pos)
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



	size_t CompressBinaryWriteStream::GetCurPos()
	{
		return m_data->size();
	}

	bool CompressBinaryWriteStream::ZlibCompress()
	{
		int ret;
		uLong uncomlen=(uLong)m_data->size()-(BINARY_PACKLEN_LEN_2 + RATIO_LEN);
		uLong comprLen= (uLong)((uncomlen + 12)*1.001+1);
		char * out_;
		char * depdest = NULL;
		char stackdest[65535];
		if(comprLen>65500)
		{
			depdest=new (std::nothrow)char[comprLen];
			out_=depdest;
		}
		else
		{
			out_=stackdest;
		}
		Byte* destbuf=(Byte*)(out_);
		ret =compress(destbuf, &comprLen, (const Bytef*)(m_data->data()+(BINARY_PACKLEN_LEN_2 + RATIO_LEN)),uncomlen);
		if(Z_OK!=ret)
		{
			if(depdest)
			{
				delete [] depdest;
				depdest = NULL;
			}
			return false;
		}
		uLong packlen=comprLen+(BINARY_PACKLEN_LEN_2 + RATIO_LEN);
		uint32_t i_packlen = packlen;
		i_packlen = htonl(i_packlen);
		m_data->assign((char*)&i_packlen,sizeof(uint32_t));
		Byte zlibflag = ZlibTag;
		m_data->append((char*)&zlibflag,sizeof(Byte));
		int16_t ratio = uncomlen*10/comprLen+1;
		ratio = htons(ratio);
		m_data->append((char*)&ratio,sizeof(int16_t));
		m_data->append((char*)destbuf,comprLen);
		if(depdest)
		{
			delete [] depdest;
			depdest = NULL;
		}
		return true;
	}


	size_t CompressBinaryWriteStream::GetSize() const
	{
		return m_data->size();
	}

	const char* CompressBinaryWriteStream::GetData() const
	{
		return m_data->data();
	}


	void CompressBinaryWriteStream::Clear()
	{
		m_data->clear();
		char buf[7];
		m_data->append(buf,sizeof(buf));
	}


	void CompressBinaryWriteStream::Flush(OPTFLAG opt)
	{
		if(opt==ADAPTIVE)
		{
			if(GetSize()>THRESHOLD)
			{
				ZlibCompress();
				return;
			}
		}
		else if(opt==COMPRESS)
		{
			ZlibCompress();
			return;
		}
		unsigned int ulen = htonl(m_data->length());
		char *ptr = &(*m_data)[0];
		memcpy(ptr,&ulen,sizeof(ulen));
		Byte zlibflag = NotZlibTag;
		memcpy(ptr+BINARY_PACKLEN_LEN_2,&zlibflag,sizeof(Byte));
		int16_t ratio = 0;
		memcpy(ptr+BINARY_PACKLEN_LEN_2+1,&ratio,sizeof(int16_t));
	}

	CompressBinaryWriteStream:: ~CompressBinaryWriteStream()
	{

	}

	CompressBinaryReadStream::CompressBinaryReadStream(const char* ptr_,size_t len_)
		: ptr(ptr_),cur(ptr_),length(len_),dest(NULL)
	{

		if(isZlibCompress())
			ZlibUnCompress();
		else
			cur=ptr+BINARY_PACKLEN_LEN_2 + RATIO_LEN;

	}


	bool  CompressBinaryReadStream:: ZlibUnCompress()	
	{
		uLong comprLen=length-BINARY_PACKLEN_LEN_2 - RATIO_LEN;
		int headlen = *(int*)ptr;
		headlen = ntohl(headlen);

		int16_t ratio= *(int16_t *)(ptr+BINARY_PACKLEN_LEN_2+1);
		ratio = ntohs(ratio);
		uLong uncomprLen=ratio *comprLen/10+1;
		if(uncomprLen>65500)
		{      
			dest=new char[uncomprLen+BINARY_PACKLEN_LEN_2 + RATIO_LEN];
			if(!dest)
				return false;
			memcpy(dest,ptr,BINARY_PACKLEN_LEN_2);
			Bytef* destbuf=(Bytef*)(dest+BINARY_PACKLEN_LEN_2 + RATIO_LEN);
			int ret=uncompress(destbuf,&uncomprLen,(const Bytef*)(ptr+BINARY_PACKLEN_LEN_2 + RATIO_LEN),comprLen);
			if(Z_OK==ret)
			{

				ptr=dest;
				cur=dest+BINARY_PACKLEN_LEN_2 + RATIO_LEN;
				length=uncomprLen+BINARY_PACKLEN_LEN_2 + RATIO_LEN;

				return true;
			}
		}
		else
		{
			Byte *source = (Byte *)(ptr + BINARY_PACKLEN_LEN_2 + RATIO_LEN);
			int ret=uncompress((Byte*)(stackdest+ BINARY_PACKLEN_LEN_2 + RATIO_LEN),&uncomprLen,source,comprLen);
			if(Z_OK==ret)
			{
				ptr= stackdest;	  	 
				cur= stackdest+BINARY_PACKLEN_LEN_2 + RATIO_LEN;
				length=uncomprLen+BINARY_PACKLEN_LEN_2 + RATIO_LEN;
				return true;
			}
		}
		return false;
	}



	bool  CompressBinaryReadStream::isZlibCompress()
	{
		Byte flag;
		memcpy(&flag,ptr+BINARY_PACKLEN_LEN_2,sizeof(char));   
		return (flag==ZlibTag)?true:false;
	}   

	bool CompressBinaryReadStream::Read(short &i)
	{
		int64_t i2=i;
		if(!Read(i2))
			return false;
		i = i2;
		return true;
	}

	bool CompressBinaryReadStream::Read(unsigned short  &i)
	{
		uint64_t i2=i;
		if(!Read(i2))
			return false;
		i = i2;
		return true;
	}

	bool CompressBinaryReadStream::Read(char &c)
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

	bool CompressBinaryReadStream::IsNULL()
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


	bool CompressBinaryReadStream::Read(int &i)
	{
		int64_t i2 = i;
		if(!Read(i2))
			return false;
		i = i2;
		return true;
	}

	bool CompressBinaryReadStream::Read(unsigned int &i)
	{
		uint64_t i2=i;
		if(!Read(i2))
			return false;
		i = i2;
		return true;
	}

	bool CompressBinaryReadStream::Read(int64_t &i)
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

	bool CompressBinaryReadStream::Read(uint64_t &i)
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

	bool CompressBinaryReadStream::UnCompress(int64_t &i,const char *buf,size_t &len)
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


	bool CompressBinaryReadStream::U_UnCompress(uint64_t &i,const char *buf,size_t &len)
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

	bool CompressBinaryReadStream::Read(double &d)
	{
		if(*cur>>7==0)
		{
			string sd;
			size_t slen;
			if(!Read(&sd,0,slen))
				return false;
			d = strtod(sd.c_str(),NULL);
			return true;
		}
		char flag;
		if(!Read(flag))
			return false;
		int64_t index=flag&0x1f;
		int64_t i;
		if(!Read(i))
			return false;
		double p = pow(10,-index);
		d = i*p;
		return true;
	}
	bool CompressBinaryReadStream::ReadDouble(string &sd)
	{
		if(*cur>>7==0)
		{
			string stemp;
			size_t slen;
			if(!Read(&stemp,0,slen))
				return false;
			size_t findpos=stemp.find(".");
			int count =0;
			if(findpos!=string::npos)
			{
				for(int i=slen-1;i >=0;i--)
				{
					if(stemp[i]!='0')
					{
						if((size_t)i==findpos)
							count++;
						break;
					}
					else 
					{
						count++;
					}
				}
			}
			sd.assign(stemp.data(),slen-count);
			return true;
		}	
		char flag;
		if(!Read(flag))
			return false;
		int64_t index=flag&0x1f;
		int64_t i;
		if(!Read(i))
			return false;
		double p = pow(10,-index);
		double d = i*p;
		char buf[256];
		if(index !=0)
		{
			sd=NumberToString(d);
			bool isFindE = false;
			size_t findpos;
			findpos = sd.find("e");
			if(findpos != string::npos)
				isFindE = true;
			if(isFindE)
			{
				char buf[256];
				sprintf(buf,"%.18f",d);
				string stemp;
				stemp.assign(buf,strlen(buf));
				size_t slen=stemp.length();
				size_t findpos=stemp.find(".");
				int count =0;
				if(findpos!=string::npos)
				{
					for(int i=slen-1;i >=0;i--)
					{
						if(stemp[i]!='0')
						{
							if((size_t)i==findpos)
								count++;
							break;
						}
						else 
						{
							count++;
						}
					}
				}
				sd.assign(stemp.c_str(),slen-count);
			}
			return true;
		}
		else
		{
			int64_t i = (int64_t)d;
			sprintf(buf,"%ld" ,i);
		}
		sd.assign(buf,strlen(buf));
		return true;
	}


	bool CompressBinaryReadStream:: Read(char *str,size_t maxlen, size_t &outlen)
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


	bool CompressBinaryReadStream:: Read(string *str,size_t maxlen,size_t& outlen)
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

	bool CompressBinaryReadStream::Read(const char** str,size_t maxlen,size_t& outlen)
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


	size_t CompressBinaryReadStream::GetSize() const
	{
		return length;
	}

	const char* CompressBinaryReadStream::GetData() const
	{
		return ptr;
	}

	CompressBinaryReadStream:: ~CompressBinaryReadStream()
	{
		if (dest)
		{
			delete[]dest;
			dest=NULL;
		}

	}

}






























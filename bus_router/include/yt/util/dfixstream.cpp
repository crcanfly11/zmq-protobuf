#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "yt/util/dfixstream.h"

namespace yt
{
	DFixReadStream::DFixReadStream(const char *ptr_,size_t len_)
		: ptr(ptr_),len(len_){}

	const char* DFixReadStream::GetData() const
	{
		return ptr;
	}
	size_t DFixReadStream::GetSize() const
	{
		return len;
	}
	bool DFixReadStream::GetValue(const char *tag,char **buf,size_t &buflen,DFIXVALUETYPE *type)
	{
		map<string,DFixValue>::iterator it = m.find(tag);
		if(it == m.end())
		{
			return false;
		}
		*buf = it->second.buf;
		buflen = it->second.buflen;
		if(type)
			*type = it->second.type;
		return true;
	}
	bool DFixReadStream::Parse()
	{
		m.clear();
		char *cur = (char*)ptr;

		char *tagbegin = cur;
		char *tagend;
		char *databegin;
		char *dataend;

		string tag;

		while(cur < ptr + len)
		{
			if(*cur == '=')
			{
				tagend = cur;
			}
			else
			{
				cur++;
				continue;
			}
			tag.clear();
			tag.append(tagbegin,tagend - tagbegin);
			
			if(tag[0] != 'R')
			{
				if(cur + 1 > ptr + len)
				{
					return false;
				}
				databegin = ++cur;
				while(cur < ptr + len)
				{
					if(*cur == SOH)
					{
						dataend = cur;
						DFixValue dfixvalue;
						dfixvalue.buf = databegin;
						dfixvalue.buflen = dataend - databegin;
						dfixvalue.type = TEXT;
						
						m.insert(make_pair(tag,dfixvalue));
						cur++;
						break;
					}
					cur++;
				}
				tagbegin = cur;
			}
			else//R6=10|6=AAAAAAAAAA|
			{
				/*if(tag[1] == 'E')
				{
					
				}
				else if(tag[1] == 'C')
				{

				}
				else
				{*/
					tag.erase(0,1);
					char lenstr[10];
					while(cur < ptr + len)
					{
						if(*cur == SOH)
						{
							databegin = tagend + 1;
							dataend = cur;
							cur++;
							break;
						}
						cur++;
					}
					size_t rawdatalenstrlen = dataend - databegin;
					if(rawdatalenstrlen >= 9)
					{
						return false;
					}
					memcpy(lenstr,databegin,rawdatalenstrlen);
					lenstr[rawdatalenstrlen] = 0;

					int rawdatalen = atoi(lenstr);

					if(strncmp(tag.c_str(),cur,tag.length()) != 0)
					{
						return false;
					}
					cur += tag.length();
					if(*cur != '=')
					{
						return false;
					}
					cur++;

					DFixValue dfixvalue;
					dfixvalue.buf = cur;
					dfixvalue.buflen = rawdatalen;
					dfixvalue.type = RAW;

					m.insert(make_pair(tag,dfixvalue));

					cur += rawdatalen;
					if(*cur != SOH)
					{
						return false;
					}
					cur++;
					tagbegin = cur;
				//}
			}
		}
		return true;
	}



	DFixWriteStream::DFixWriteStream(char *ptr_,size_t len_)
		: ptr(ptr_),len(len_),begin(ptr + MAXHEADERLEN),cur(begin),hdrbegin(NULL)
	{
		if(len_ < 32)
			assert(0);
	}

	const char* DFixWriteStream::GetData() const{
		if(!hdrbegin)
			assert(0);
		return hdrbegin;
	}
	size_t DFixWriteStream::GetSize() const{
		if(!hdrbegin)
			assert(0);
		return cur - hdrbegin;
	}
			
	bool DFixWriteStream::AddInt(const char *tag,int i)
	{
		char buf[10];
		sprintf(buf,"%d",i);
		return AddText(tag,buf,strlen(buf));
	}
	bool DFixWriteStream::AddText(const char *tag,const char *buf,size_t buflen)
	{
		size_t taglen = strlen(tag);
		if(cur + taglen + 1 + buflen + 1 > ptr + len) //8=A1|
			return false;

		memcpy(cur,tag,taglen);
		cur += taglen;

		*cur = '=';
		cur++;

		memcpy(cur,buf,buflen);
		cur += buflen;

		*cur = SOH;
		cur++;

		return true;
	}
	bool DFixWriteStream::AddRaw(const char *tag,const char *buf,size_t buflen)
	{
		size_t taglen = strlen(tag);
		char lenstr[10];
		sprintf(lenstr,"%ld",buflen);
		size_t lenstrlen = strlen(lenstr);

		if(cur + 1 + taglen + 1 + lenstrlen + 1 + taglen + 1 + buflen + 1 > ptr + len)//R6=10|6=AAAAAAAAAA|
			return false;

		*cur = 'R';
		cur++;
		
		strcpy(cur,tag);
		cur += taglen;

		*cur = '=';
		cur++;

		strcpy(cur,lenstr);
		cur += lenstrlen;

		*cur = SOH;
		cur++;

		strcpy(cur,tag);
		cur += taglen;

		*cur = '=';
		cur++;

		memcpy(cur,buf,buflen);
		cur += buflen;

		*cur = SOH;
		cur++;

		return true;
	}
	bool DFixWriteStream::Flush()
	{
		char lenstr[10];
		sprintf(lenstr,"%ld",cur - begin);

		size_t lenstrlen = strlen(lenstr);
		if(lenstrlen > MAXHEADERLEN - 3)
			return false;

		//if(cur + 1 + 1 + lenstrlen + 1 > ptr + len)//L=1024|......
		//	return false;
		
		hdrbegin = begin - 3 - lenstrlen;
		char *hdrcur = hdrbegin;

		strcpy(hdrcur,"L=");
		hdrcur += 2;
		strcpy(hdrcur,lenstr);
		hdrcur += lenstrlen;
		*hdrcur = SOH;

		return true;
	}
	void DFixWriteStream::Clear()
	{
		cur = begin;
		hdrbegin = NULL;
	}
}

#include "dbresult.h"
#include "yt/log/log.h"

namespace yt
{
	bool DBResult::GetDBResult(ReadStreamImpl &stream)
	{
		if(!stream.Read(retfromdb))
		{
			AC_ERROR("read ret from dbagent error");
			return false;
		}
		if(retfromdb < 0)
		{
			AC_ERROR("retfromdb = %d",retfromdb);
			return false;
		}
		if(!stream.Read(paramcount))
		{
			AC_ERROR("read paramcount error");
			return false;
		}
		if(paramcount != 0)
		{
			AC_ERROR("param != 0");	
			return false;
		}

		if(!stream.Read(result,sizeof(result),resultlen))
		{
			AC_ERROR("read info error");
			return false;
		}

		return true;
	}

	bool ResultSet::ParseResult(const char* result,size_t resultlen)
	{
		BinaryReadStream infostream(result,resultlen);
		if(!infostream.Read(rownum))
		{
			AC_ERROR("read row num error");
			return false;
		}
		if(!infostream.Read(colnum))
		{
			AC_ERROR("read col num error");
			return false;
		}

		static char buf[65535];
		char *ptr = buf;
		size_t curlen;
		for(int i = 0;i < rownum;i++)
		{
			for(int j = 0;j < colnum;j++)
			{
				if(size_t(ptr - buf) > sizeof(buf))
				{
					AC_ERROR("ptr error");
					return false;
				}
				if(!infostream.Read(ptr,sizeof(buf),curlen))
				{
					AC_ERROR("read ptr error");
					return false;
				}
				ptr[curlen] = 0;
				resultmap[make_pair(i,j)] = make_pair(ptr,curlen+1);
				ptr += (curlen+1);
			}
		}
		return true;
	}

	bool ResultSet::GetResult(int row,int col,char** buf,size_t &buflen)
	{
		map< pair<int,int>, pair<char*,size_t> >::iterator it = resultmap.find(make_pair(row,col));
		if(it == resultmap.end())
			return false;
		*buf = it->second.first;
		buflen = it->second.second;
		return true;
	}

	bool ResultSet::Read(int row,int col,char* buf,size_t buflen)
	{
		char *pbuf;
		size_t pbuflen;

		map< pair<int,int>, pair<char*,size_t> >::iterator it = resultmap.find(make_pair(row,col));
		if(it == resultmap.end())
			return false;
		pbuf = it->second.first;
		pbuflen = it->second.second;

		if(pbuflen > buflen)	return false;
		//memset(buf,0,buflen);
		memcpy(buf,pbuf,pbuflen);

		return true;
	}

}

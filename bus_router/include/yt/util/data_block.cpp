#include <string.h>
#include "yt/util/data_block.h"
#include "yt/util/freesizememallocator.h"
#include "yt/log/stacktrace.h"
#include "yt/log/log.h"
#include "yt/util/hashfun.h"

namespace yt
{
	DataBlock::DataBlock(size_t maxsize,size_t expandsize)
		:m_buf(NULL),m_maxsize(maxsize),m_expandsize(expandsize),m_start_pos(0),m_end_pos(0),m_pos(0),m_size(0),m_mode(0)
		{
			//_AC_STACK_TRACE();
		}

	void DataBlock::Init(size_t maxsize,size_t expandsize)
	{
		m_maxsize = maxsize;
		m_expandsize = expandsize;
		m_pos = 0;
		m_start_pos = 0;
		m_end_pos = 0;
	}
	DataBlock::~DataBlock()
	{
		//printf("~DataBlock\n");
		if(m_buf)
			free(m_buf);
	}
	int DataBlock::Append(const char *buf,size_t buflen)
	{
	    //数据长度小于缓存剩余空间，直接append，超过长度，将数据移至buffer的首位置
		if( m_end_pos + buflen <= m_size )
        {
        	memcpy( m_buf + m_end_pos, buf, buflen);
			m_end_pos += buflen;
			m_pos += buflen;
			return 0;
		}
		
		//数据长度超过了缓存剩余空间，先将缓存内数据移动到缓存首地址，在进行append
		if( m_start_pos > 0 )
		{
			char *pstart = m_buf + m_start_pos;
			memcpy( m_buf, pstart, m_pos );
			m_start_pos = 0;
			m_end_pos = m_pos;
		}
		
		size_t new_size = m_pos + buflen;
		if(Expand(new_size) != 0)
			return -1;
		return Append( buf, buflen );	
	}

	
	int DataBlock::Copy(size_t pos,const char *buf,size_t buflen)
	{
	    //buf在缓存区内部，改变下m_start_pos,m_end_pos,m_pos即可
		if( buf >= m_buf + m_start_pos && buf < m_buf + m_end_pos )
		{
		   m_start_pos= buf - m_buf;
		   m_pos = buflen;
		   m_end_pos = m_start_pos + m_pos;
		   return 0;
		}
		
		//外部指针
		size_t newpos = pos +  buflen;

		if(Expand(newpos) != 0)
			return -1;

		memcpy(m_buf + pos,buf,buflen);
		m_pos = newpos;

		m_start_pos= pos;
		m_pos = buflen;
		m_end_pos = m_start_pos + m_pos;
		return 0;
	}
	int DataBlock::Expand(size_t buflen)
	{
		if(buflen <= m_size)
			return 0;
		size_t expandsize = FreeSizeMemAllocator::GetFitSize(buflen,m_expandsize);
		if(m_mode == 0)
		{
			if(expandsize > m_maxsize * 10)
				return -1;
		}
		else
		{
			if(expandsize > m_maxexpandsize2)
				return -1;
		}
		char *buf = (char*)realloc(m_buf,expandsize);
		if(!buf)
			return -1;
		m_size = expandsize;
		m_buf = buf;
		return 0;
	}
	void DataBlock::InitPos()
	{
		m_pos = 0;
		m_start_pos = 0;
		m_end_pos = 0;
		if(m_size > m_maxsize)
		{
			free(m_buf);
			m_buf = NULL;
			m_size = 0;
		}
	}
	
	DataBlock2::DataBlock2(size_t maxsize,size_t expandsize)
		:m_buf(NULL),m_maxsize(maxsize),m_expandsize(expandsize),m_bgpos(0),m_endpos((size_t)-1),m_size(0),m_mode(0){ }

	DataBlock2::~DataBlock2()
	{
		if(m_buf != NULL)
		{
			free(m_buf);
			m_buf = NULL;
		}
	}
	
	void DataBlock2::Init(size_t maxsize,size_t expandsize)
	{
		m_maxsize = maxsize;
		m_expandsize = expandsize;
	}

	size_t DataBlock2::GetLen()
	{
		if (m_endpos == (size_t)-1)
			return 0;
		if (m_endpos == m_bgpos)
			return m_size;
		return (m_size + m_endpos - m_bgpos) % m_size;
	}

	//-- Modified by xuzhiwen 2012/8/21 --
	void DataBlock2::copy(const char *buf, size_t buflen, ECircleCacheMode mode /*= RECV_CIRCLE_CACHE*/)
	{
		//printf("copy1: m_bgpos[%d], m_endpos[%d], m_size[%d]\n", m_bgpos, m_endpos, m_size);
		if (m_endpos == (size_t)-1)
		{
			memcpy(m_buf + m_bgpos, buf, buflen);
			m_endpos = (m_bgpos + buflen) % m_size;
		}
		else
		{
			if ((m_endpos + buflen) > m_size)
			{
				if (mode == SEND_CIRCLE_CACHE)
				{
					size_t size2 = (m_endpos + buflen) % m_size;
					size_t size1 = buflen - size2;
					memcpy(m_buf + m_endpos, buf, size1);
					memcpy(m_buf, buf + size1, size2);
					m_endpos = size2;
				}
				else if (mode == RECV_CIRCLE_CACHE)
				{
					char* pBuf = NULL;
					size_t nBufLen = 0;
					if (GetBuf(&pBuf, nBufLen) == 0)
					{
						if (pBuf != NULL && nBufLen != 0)
						{
							memmove(m_buf, pBuf, nBufLen);
							memcpy(m_buf + nBufLen, buf, buflen);
							m_bgpos = 0;
							m_endpos = nBufLen + buflen;
//							size_t nTotalBufLen = nBufLen + buflen;
//							m_endpos = (m_bgpos + nTotalBufLen) % m_size;
							/*AC_INFO("circle cache tail copy:source buffer length(%ul)," \
								    "dest buffer length(%ul),total length(%ul).  \n",
									nBufLen,
									buflen,
									m_endpos);
									*/
						}
					}
				}
			}
			else
			{
				memcpy(m_buf + m_endpos, buf, buflen);
				m_endpos = (m_endpos + buflen) % m_size;
			}
		}
		//printf("copy2: m_bgpos[%d], m_endpos[%d], m_size[%d]\n", m_bgpos, m_endpos, m_size);
	}

	int DataBlock2::Append(const char *buf, size_t buflen, ECircleCacheMode mode /*= RECV_CIRCLE_CACHE*/)
	{
		if (buflen == 0)
			return 0;
	
		size_t newsize = GetLen() + buflen;
		//printf("Append1: bgpos[%d], endpos[%d], newsize[%d]\n", m_bgpos, m_endpos, newsize);

		if(Expand(newsize) != 0)
			return -1;

		copy(buf, buflen, mode);
		//printf("Append2: bgpos[%d], endpos[%d], m_size[%d]\n", m_bgpos, m_endpos, m_size);
		return 0;
	}
	//-- Modified end --

	int DataBlock2::Expand(size_t size)
	{
		if(size <= m_size)
			return 0;

		size_t power;
		size_t expandsize = GetFitSize(size, 64, 1.25, power);

		if(m_mode == 0)
		{
			if(expandsize > m_maxsize * 10)
			{
				if ((m_size < m_maxsize * 10) && (size < m_maxsize * 10))
				{
					expandsize = m_maxsize * 10;
				}
				else
				{
					return -1;
				}
			}
		}
		else
		{
			if(expandsize > m_maxexpandsize2)
			{
				if ((m_size < m_maxexpandsize2) && (size < m_maxexpandsize2))
				{
					expandsize = m_maxexpandsize2;
				}
				else
				{
					return -1;
				}
			}
		}

		char *buf = (char*)malloc(expandsize);
		//memset(buf, '0', expandsize);
		if(!buf)
        {
            AC_ERROR("malloc error");
			return -1;
        }
			
		if (m_endpos != (size_t)-1)
		{
			if (m_endpos > m_bgpos)
			{
				size_t len = m_endpos - m_bgpos;
				memcpy(buf, m_buf + m_bgpos, len);
				m_bgpos = 0;
				m_endpos = len;
			}
			else
			{
				size_t size1 = m_size - m_bgpos;
				size_t size2 = m_endpos;

				memcpy(buf, m_buf + m_bgpos, size1);
				if (size2 > 0)
					memcpy(buf + size1, m_buf, size2);
				
				m_endpos = GetLen();
				m_bgpos = 0;
			}
		}
		
		if (m_buf != NULL)
		{
			free(m_buf);
			m_buf = NULL;
		}

		m_size = expandsize;
		m_buf = buf;

		return 0;
	}

	int DataBlock2::GetBuf(char **buf, size_t &size) 
	{
		if (m_endpos == (size_t)-1)
			return -1;

		if (m_endpos > m_bgpos)
		{
			size = m_endpos - m_bgpos;
		}
		else
		{
			size = m_size - m_bgpos;
		}

		*buf = m_buf + m_bgpos;

		return 0;
	}

	void DataBlock2::move(size_t size)
	{
		if (m_endpos == (size_t)-1)
			return;

		if (m_endpos > m_bgpos)
		{
			if (size >= (m_endpos - m_bgpos))
				InitPos();
			else
				m_bgpos += size;
		}
		else
		{
			if ((m_size - m_bgpos) >= size)
			{
				m_bgpos = (m_bgpos + size) % m_size;
			}
			else
			{
				size_t size1 = m_size - m_bgpos;
				size_t size2 = size - size1;
				m_bgpos = (size2 > m_endpos) ? m_endpos : size2;
			}

			if (m_bgpos == m_endpos)
				InitPos();
		}
		//printf("move: bgpos[%d], endpos[%d]\n", m_bgpos, m_endpos);
	}

	void DataBlock2::InitPos()
	{
		m_bgpos = 0;
		m_endpos = (size_t)-1;
		
		if(m_size > m_maxsize)
		{
			if (m_buf != NULL)
			{
				free(m_buf);
				m_buf = NULL;
				m_size = 0;
			}
		}
	}
    
    int DataBlock2::Copy(int i, const char *src, size_t len)
    {
        if (m_size == 0)
        {
            Append(src, len);
            return 0;
        }

		//-- Modified by xuzhiwen 2012/8/23 --
		size_t nEndPosLen = GetLen();  // get the effective datas length
        const char *bgptr = m_buf + m_bgpos;
//        const char *endptr = m_buf + nEndPosLen;
		const char *endptr = bgptr + nEndPosLen;
		//-- Modified end --
        
        if ((bgptr <= src) && (endptr > src))
        {
            move(src - bgptr);
			return 0;
        }
        else
            return Append(src, len);
        
//        return 0;
    }
}


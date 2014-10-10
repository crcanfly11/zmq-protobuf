#pragma once

#include <sys/types.h>
#include <stdio.h>

namespace yt
{
/*
可扩张的缓冲区类,用于socket的发送和接收缓冲区
*/
	enum {MAX_BLOCK_SIZE = 1048576,BLOCK_EXPAND_SIZE = 1024};
	enum ECircleCacheMode{ SEND_CIRCLE_CACHE = 1, RECV_CIRCLE_CACHE };  // add by xuzhiwen 2012/8/21


	class DataBlock
	{
		public:
			DataBlock(size_t maxsize = MAX_BLOCK_SIZE,size_t expandsize = BLOCK_EXPAND_SIZE);
			void Init(size_t maxsize = MAX_BLOCK_SIZE,size_t expandsize = BLOCK_EXPAND_SIZE);
			virtual ~DataBlock();
			inline void SetExpandSize(size_t expandsize){m_expandsize = expandsize;}
			inline void SetMaxSize(size_t maxsize){m_maxsize = maxsize;}
			int Append(const char *buf,size_t buflen);
			inline char* GetBuf() const {return m_buf + m_start_pos;}
			inline size_t GetPos() const {return m_pos;}
			inline size_t GetSize() const {return m_size;}
			inline void SetMode2(size_t maxexpandsize = 104857600 * 2){
				m_mode = 1;
				m_maxexpandsize2 = maxexpandsize;
			}
			void InitPos();
			int Copy(size_t pos,const char *buf,size_t buflen);
			int Expand(size_t buflen);
		private:	
			
			char *m_buf;

			size_t m_maxsize;
			size_t m_expandsize;
			size_t m_maxexpandsize2;

			size_t m_start_pos;//绝对pos
			size_t m_end_pos;  //绝对pos
			size_t m_pos;      //相对pos
			size_t m_size;
			int m_mode;
	};
	
	class DataBlock2
	{	
		public:		
			DataBlock2(size_t maxsize = MAX_BLOCK_SIZE,size_t expandsize = BLOCK_EXPAND_SIZE);
			~DataBlock2();
			void Init(size_t maxsize = MAX_BLOCK_SIZE,size_t expandsize = BLOCK_EXPAND_SIZE);
			void InitPos();
			int GetBuf(char **buf, size_t &size);
			int Append(const char *buf,size_t buflen, ECircleCacheMode mode = RECV_CIRCLE_CACHE);  // add by xuzhiwen 2012/8/21
			void move(size_t size);
			bool empty() { return GetLen() == 0; }
			
			inline void SetExpandSize(size_t expandsize){m_expandsize = expandsize;}
			inline void SetMaxSize(size_t maxsize){m_maxsize = maxsize;}
			inline size_t GetSize() const {return m_size;}

			inline void SetMode2(size_t maxexpandsize = 104857600 * 2){
				m_mode = 1;
				m_maxexpandsize2 = maxexpandsize;
			}
			size_t GetLen();

            // 接收缓冲区接口
            int Copy(int i, const char *src, size_t len); 	


		private:
			int Expand(size_t buflen);
			void copy(const char *buf, size_t buflen, ECircleCacheMode mode = RECV_CIRCLE_CACHE);  // modified by xuzhiwen 2012/8/21
			
		private:
			char *m_buf;					// buffer的起始位置
			size_t m_maxsize;				// 调整buffer大小的一个权值
			size_t m_expandsize;			// 环的最小值
			size_t m_maxexpandsize2;		// 用于mode=1, 表示环的最大值

			size_t m_bgpos;					// 有效数据 [m_bgpos, m_endpos)
			size_t m_endpos;				// m_endpos == m_bgpos, 环满
			size_t m_size;					// 环大小
			int m_mode;
	};
}


#pragma once

#include <map>
#include "yt/util/protocolstream.h"
#include "yt/util/data_block.h"

using namespace std;

namespace yt
{
	class ResultSet
	{
		public:
			short rownum;
			short colnum;
		private:
			map< pair<int,int>, pair<char*,size_t> > resultmap;
			bool GetResult(int row,int col,char** buf,size_t &buflen);
		public:
			bool ParseResult(const char* result,size_t resultlen);
			bool Read(int row,int col,char* buf,size_t buflen);
			bool Read(int row,int col,int &i){
				char str[10];
				if(!Read(row,col,str,sizeof(str)))
					return false;
				i = atoi(str);
				return true;
			}	
			bool Read(int row,int col,short &i){
				char str[10];
				if(!Read(row,col,str,sizeof(str)))
					return false;
				i = (short)atoi(str);
				return true;
			}
			bool Read(int row,int col,long &i){
				char str[32];
				if(!Read(row,col,str,sizeof(str)))
					return false;
				i = atol(str);
				return true;
			}
	};

	class DBResult
	{
		public:
			int retfromdb;
			short paramcount;//0
			char result[MAX_BLOCK_SIZE];
			size_t resultlen;
			ResultSet resultset;
		public:
			bool GetDBResult(ReadStreamImpl &stream);
			bool ParseResult()
			{
				return resultset.ParseResult(result,resultlen);
			}
	};

	class DBBounder
	{
		public:
			virtual ~DBBounder(){}
			virtual bool ReadFromResultSet(int row,ResultSet *resultset) = 0;
			virtual void PutIntoStream(WriteStreamImpl &stream) = 0;
	};
}

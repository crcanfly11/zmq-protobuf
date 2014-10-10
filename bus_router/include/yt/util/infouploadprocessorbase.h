#pragma once

#include "infouploadmanager.h"
namespace yt
{
	class InfoUploadProcessorBase
	{
		public :
			InfoUploadProcessorBase(){};
			virtual ~InfoUploadProcessorBase(){};

		public:
			virtual bool ProcessInfo(InfoStruct_t &info) = 0;
			virtual bool CreateJson(string &outjsonstr) = 0;

	};
}




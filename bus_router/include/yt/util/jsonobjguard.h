#pragma once
#include <json.h>

namespace yt
{
/*
	json_objectµÄautoptr
*/
	class JsonObjGuard
	{
		public:
			JsonObjGuard(json_object *obj) : m_obj(obj){}
			~JsonObjGuard(){
				json_object_put(m_obj);
			}
		private:
			json_object *m_obj;
	};
}

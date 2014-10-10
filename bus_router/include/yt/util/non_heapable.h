#pragma once

#include <stdio.h>

namespace yt
{
	//继承这种类的类不能new
	class NonHeapable
	{
		private:
			void * operator new(size_t/* size*/) throw() {
				return NULL;
			}
			void operator delete(void* /*p*/){
			}
	};
}


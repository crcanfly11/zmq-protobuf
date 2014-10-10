#pragma once

namespace yt
{
/*
ËøµÄ»ùÀà
*/
	class Lockable
	{
		public:
			virtual ~Lockable(){}
			virtual int Release() = 0;
	};
}

#pragma once

namespace yt
{
	//继承这种类的类不能再栈上分配
	class NonStackable
	{
		private:
			//NonStackable() {}
			~NonStackable() {}
	};
}


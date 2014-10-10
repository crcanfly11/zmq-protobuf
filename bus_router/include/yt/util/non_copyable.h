#pragma once

namespace yt
{
	/*! \brief 所有要求不可赋值(拷贝构造)的类都可以继承于此
	 *  \note 摘自IceUtil::::noncopyable
	 */
	class NonCopyable
	{

		protected:
			NonCopyable() {}
			~NonCopyable() {}

		private:
			NonCopyable(const NonCopyable &);
			const NonCopyable & operator=(const NonCopyable &);
	};
}


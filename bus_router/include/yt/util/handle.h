#pragma once

namespace yt {
	struct Handle
	{
		int hdl;

		Handle(int hdl_ = -1)
			: hdl(hdl_)
			{
			}

		operator int() const
		{
			return hdl;
		}
	};
} // namespace yt


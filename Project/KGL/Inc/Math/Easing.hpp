#pragma once

namespace KGL
{
	inline namespace MATH
	{
		inline namespace EASE
		{
			inline float InCubic(float x) noexcept
			{
				return x * x * x;
			}
		}
	}
}
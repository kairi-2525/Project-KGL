#pragma once

#include <vector>

namespace KGL
{
	inline namespace MATH
	{
		// @param count ������
		std::vector<float> GetGaussianWeights(size_t count, float s);
	}
}
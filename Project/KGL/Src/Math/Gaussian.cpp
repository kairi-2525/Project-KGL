#include <Math/Gaussian.hpp>

using namespace KGL;

std::vector<float> MATH::GetGaussianWeights(size_t count, float s)
{
	std::vector<float> weights(count);

	float x = 0.0f;
	float total = 0.0f;
	for (auto& wgt : weights)
	{
		wgt = expf(-(x * x) / (2 * s * s));
		total += wgt;
		x += 1.f;
	}

	// ���E�ɍL����̂œ�{���Đ^�񒆂����̂ň���炷
	total = total * 2.f - 1.f;

	// �����ĂP�ɂȂ�悤�ɂ���
	for (auto& wgt : weights)
	{
		wgt /= total;
	}

	return weights;
}
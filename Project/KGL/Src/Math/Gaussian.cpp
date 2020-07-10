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

	// ¶‰E‚ÉL‚°‚é‚Ì‚Å“ñ”{‚µ‚Ä^‚ñ’†‚ª”í‚é‚Ì‚Åˆê‚ÂŒ¸‚ç‚·
	total = total * 2.f - 1.f;

	// ‘«‚µ‚Ä‚P‚É‚È‚é‚æ‚¤‚É‚·‚é
	for (auto& wgt : weights)
	{
		wgt /= total;
	}

	return weights;
}
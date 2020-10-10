#include "../Hrd/MSAA.hpp"

UINT MSAASelector::TypeToCount(TYPE scale)
{
	switch (scale)
	{
		case TYPE::MSAA_OFF: return 1u;
		case TYPE::MSAAx2: return 2u;
		case TYPE::MSAAx4: return 4u;
		case TYPE::MSAAx8: return 8u;
		case TYPE::MSAAx16: return 16u;
		case TYPE::MSAAx32: return 32u;
	}
	return 0;
}

MSAASelector::MSAASelector(UINT max_sample_count)
{
	set_scale = MSAA_OFF;

	switch (max_sample_count)
	{
		case 32u: max_scale = MSAAx32; break;
		case 16u: max_scale = MSAAx16; break;
		case 8u: max_scale = MSAAx8; break;
		case 4u: max_scale = MSAAx4; break;
		case 2u: max_scale = MSAAx2; break;
		default: max_scale = MSAA_OFF;
	}
}

void MSAASelector::SetScale(TYPE scale)
{
	if (scale <= max_scale)
	{
		set_scale = scale;
	}
	else
	{
		set_scale = max_scale;
	}
}
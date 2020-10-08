#include "../Hrd/MSAA.hpp"

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
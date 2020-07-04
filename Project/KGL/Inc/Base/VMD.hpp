#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

namespace KGL
{
	inline namespace BASE
	{
		namespace VMD
		{
			struct Header
			{
				FLOAT	version;
				CHAR	model_name[20];
				CHAR	comment[256];
			};
		}
	}
}
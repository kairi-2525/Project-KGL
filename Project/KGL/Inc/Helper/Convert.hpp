#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

namespace KGL
{
	inline namespace CONVERT
	{
		extern void CHARToTCHAR(TCHAR pDestStr[512], const char* pSrcStr) noexcept;
	}
}
#pragma once

#include <string>
#include <iostream>

#ifdef _CONSOLE
#define KGLDebugOutPutString KGL::DEBUG::OutPutString
#else
#define KGLDebugOutPutString 1 ? (void) 0 : KGL::DEBUG::OutPutString
#endif

namespace KGL
{
	namespace DEBUG
	{
		inline void OutPutString(const std::string& str)
		{
			std::cout << str << std::endl;
		}
	}
}
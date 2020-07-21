#pragma once

#include <string>
#include <iostream>

#ifdef _CONSOLE
#define KGLDebugOutPutString KGL::DEBUG::OutPutString
#define KGLDebugOutPutStringNL KGL::DEBUG::OutPutStringNL
#else
#define KGLDebugOutPutString 1 ? (void) 0 : KGL::DEBUG::OutPutString
#define KGLDebugOutPutStringNL 1 ? (void) 0 : KGL::DEBUG::OutPutStringNL
#endif

namespace KGL
{
	namespace DEBUG
	{
		inline void OutPutString(const std::string& str)
		{
			std::cout << str << std::endl;
		}
		inline void OutPutStringNL(const std::string& str)
		{
			std::cout << str;
		}
	}
}
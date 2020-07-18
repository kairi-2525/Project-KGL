#pragma once

#include <utility>
#include <string>
#include <stdexcept>
#include <cassert>

#ifdef _M_IX86
#define _X86_
#elif defined(_M_AMD64)
#define AMD64_
#endif
#include <winnt.h>


#include "Debug.hpp"

#pragma warning( disable : 4003 )
#define RCHECK(fail, msg, result) if (fail) { KGLDebugOutPutString(msg); assert(!msg); return result; }
#define RCHECK_HR(hr, msg) RCHECK(FAILED(hr), msg, hr)
#define RCHECK_STR(fail, str, msg, result) if (fail) { KGLDebugOutPutString(str); assert(!msg); return result; }
#define RCHECK_HRSTR(hr, str, msg) RCHECK_STR(FAILED(hr), str, msg, hr)

namespace KGL
{
	inline namespace THROW_ASSERT
	{
		using HRESULT_MSG = std::pair<HRESULT, std::string>;
		
		inline std::string SendErrorMsg(const HRESULT_MSG& error)
		{
			std::string err_msg;
			if (!error.second.empty())
			{
				OutputDebugStringA(error.second.c_str());
				OutputDebugStringA("\n");
			}
			LPVOID string;
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				error.first,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR)&string,
				0,
				NULL
			);
			OutputDebugString((LPCSTR)string);
			if (string != NULL)
			{
				OutputDebugString((LPCSTR)string);
				OutputDebugStringA("\n");
				err_msg = (LPCSTR)string;
			}
			LocalFree(string);
			return err_msg;
		}

		inline void RuntimeErrorStop(const std::runtime_error& exception)
		{
			OutputDebugStringA(exception.what());
			OutputDebugStringA("\n");
			MessageBoxA(NULL, exception.what(),
				"ERROR", MB_ICONERROR);
			unexpected();
		}
	}
}
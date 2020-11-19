#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <string>

namespace KGL
{
	inline namespace HELPER
	{
		inline namespace CONVERT
		{
			extern void CHARToTCHAR(TCHAR pDestStr[512], const char* pSrcStr) noexcept;

			// string(Shift_JIS)とwstringの文字コード変換
			extern std::wstring MultiToWide(const std::string& src) noexcept;
			extern std::string WideToMulti(const std::wstring& src) noexcept;

			// string(UTF-8)とwstringの文字コード変換
			extern std::wstring Utf8ToWide(const std::string& src) noexcept;
			extern std::string WideToUtf8(const std::wstring& src) noexcept;

			// string(Shift_JIS)とstring(UTF-8)の文字コード変換
			extern std::string MultiToUtf8(std::string const& src) noexcept;
			extern std::string Utf8ToMulti(std::string const& src) noexcept;
		}
	}
}
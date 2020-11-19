#include <Helper/Convert.hpp>
#include <filesystem>
#include <locale>

using namespace KGL;

void CONVERT::CHARToTCHAR(TCHAR pDestStr[512], const char* pSrcStr) noexcept
{
#ifdef _UNICODE
	// 第一引数で指定した文字コードを Unicode 文字コードに変換する
	::MultiByteToWideChar(CP_UTF8, 0, pSrcStr, -1, pDestStr, (int)((strlen(pSrcStr) + 1) * sizeof(WCHAR)));
#else
	WCHAR str[512];
	// 第一引数で指定した文字コードを Unicode 文字コードに変換する
	::MultiByteToWideChar(CP_UTF8, 0, pSrcStr, -1, str, (int)((strlen(pSrcStr) + 1) * sizeof(WCHAR)));
	// Unicode 文字コードを第一引数で指定した文字コードに変換する( CP_ACP は日本語WindowdではシフトJISコード )
	::WideCharToMultiByte(CP_ACP, 0, str, -1, pDestStr, (int)((wcslen(str) + 1) * 2), NULL, NULL);
#endif
}

// string と wstringの文字コード変換
static std::wstring StrToWstr(const std::string& src, int code_page)
{
	auto const dest_size = ::MultiByteToWideChar(code_page, 0U, src.data(), -1, nullptr, 0U);
	std::vector<wchar_t> dest(dest_size, L'\0');
	if (::MultiByteToWideChar(code_page, 0U, src.data(), -1, dest.data(), dest.size()) == 0) {
		throw std::system_error{ static_cast<int>(::GetLastError()), std::system_category() };
	}
	dest.resize(std::char_traits<wchar_t>::length(dest.data()));
	dest.shrink_to_fit();
	return std::wstring(dest.begin(), dest.end());
}
static std::string WstrToStr(const std::wstring& src, int code_page)
{
	auto const dest_size = ::WideCharToMultiByte(code_page, 0U, src.data(), -1, nullptr, 0, nullptr, nullptr);
	std::vector<char> dest(dest_size, '\0');
	if (::WideCharToMultiByte(code_page, 0U, src.data(), -1, dest.data(), dest.size(), nullptr, nullptr) == 0) {
		throw std::system_error{ static_cast<int>(::GetLastError()), std::system_category() };
	}
	dest.resize(std::char_traits<char>::length(dest.data()));
	dest.shrink_to_fit();
	return std::string(dest.begin(), dest.end());
}

// string(Shift_JIS)とwstringの文字コード変換
std::wstring CONVERT::MultiToWide(const std::string& src) noexcept
{
	return StrToWstr(src, CP_ACP);
}
std::string CONVERT::WideToMulti(const std::wstring& src) noexcept
{
	return WstrToStr(src, CP_ACP);
}

// string(UTF-8)とwstringの文字コード変換
std::wstring CONVERT::Utf8ToWide(const std::string& src) noexcept
{
	return StrToWstr(src, CP_UTF8);
}
std::string CONVERT::WideToUtf8(const std::wstring& src) noexcept
{
	return WstrToStr(src, CP_UTF8);
}

// string(Shift_JIS)とstring(UTF-8)の文字コード変換
std::string CONVERT::MultiToUtf8(std::string const& src) noexcept
{
	const auto wide = MultiToWide(src);
	return WideToUtf8(wide);
}
std::string CONVERT::Utf8ToMulti(std::string const& src) noexcept
{
	const auto wide = Utf8ToWide(src);
	return WideToMulti(wide);
}
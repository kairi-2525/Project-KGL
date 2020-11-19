#include <Helper/Convert.hpp>
#include <filesystem>
#include <locale>

using namespace KGL;

void CONVERT::CHARToTCHAR(TCHAR pDestStr[512], const char* pSrcStr) noexcept
{
#ifdef _UNICODE
	// �������Ŏw�肵�������R�[�h�� Unicode �����R�[�h�ɕϊ�����
	::MultiByteToWideChar(CP_UTF8, 0, pSrcStr, -1, pDestStr, (int)((strlen(pSrcStr) + 1) * sizeof(WCHAR)));
#else
	WCHAR str[512];
	// �������Ŏw�肵�������R�[�h�� Unicode �����R�[�h�ɕϊ�����
	::MultiByteToWideChar(CP_UTF8, 0, pSrcStr, -1, str, (int)((strlen(pSrcStr) + 1) * sizeof(WCHAR)));
	// Unicode �����R�[�h��������Ŏw�肵�������R�[�h�ɕϊ�����( CP_ACP �͓��{��Windowd�ł̓V�t�gJIS�R�[�h )
	::WideCharToMultiByte(CP_ACP, 0, str, -1, pDestStr, (int)((wcslen(str) + 1) * 2), NULL, NULL);
#endif
}

// string �� wstring�̕����R�[�h�ϊ�
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

// string(Shift_JIS)��wstring�̕����R�[�h�ϊ�
std::wstring CONVERT::MultiToWide(const std::string& src) noexcept
{
	return StrToWstr(src, CP_ACP);
}
std::string CONVERT::WideToMulti(const std::wstring& src) noexcept
{
	return WstrToStr(src, CP_ACP);
}

// string(UTF-8)��wstring�̕����R�[�h�ϊ�
std::wstring CONVERT::Utf8ToWide(const std::string& src) noexcept
{
	return StrToWstr(src, CP_UTF8);
}
std::string CONVERT::WideToUtf8(const std::wstring& src) noexcept
{
	return WstrToStr(src, CP_UTF8);
}

// string(Shift_JIS)��string(UTF-8)�̕����R�[�h�ϊ�
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
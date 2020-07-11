#include <Helper/Convert.hpp>

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
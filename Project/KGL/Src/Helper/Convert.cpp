#include <Helper/Convert.hpp>

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
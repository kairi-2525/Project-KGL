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

			// string(Shift_JIS)��wstring�̕����R�[�h�ϊ�
			extern std::wstring MultiToWide(const std::string& src) noexcept;
			extern std::string WideToMulti(const std::wstring& src) noexcept;

			// string(UTF-8)��wstring�̕����R�[�h�ϊ�
			extern std::wstring Utf8ToWide(const std::string& src) noexcept;
			extern std::string WideToUtf8(const std::wstring& src) noexcept;

			// string(Shift_JIS)��string(UTF-8)�̕����R�[�h�ϊ�
			extern std::string MultiToUtf8(std::string const& src) noexcept;
			extern std::string Utf8ToMulti(std::string const& src) noexcept;

			// �啶���ɕϊ�
			extern std::string StrToUpper(std::string const& src) noexcept;
			// �������ɕϊ�
			extern std::string StrToLower(std::string const& src) noexcept;

			// �T�C�Y���A���C�����g����
			template<class _Ty0 = UINT, class _Ty1 = UINT>
			extern inline constexpr _Ty0 RoundUp(_Ty0 val, _Ty1 alignment_size) noexcept
			{
				alignment_size--;
				return (val + alignment_size) & ~alignment_size;
			}
		}
	}
}
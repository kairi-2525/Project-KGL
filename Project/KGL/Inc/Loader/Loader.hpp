#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <string>
#include "../Helper/ThrowAssert.hpp"

namespace KGL
{
	inline namespace LOADER
	{
		inline bool IsFound(HRESULT hr) noexcept { return hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) && hr != HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND); };
		inline void AssertLoadResult(HRESULT hr, const std::string& file_name) noexcept
		{
			try
			{
				if (!IsFound(hr))
					throw std::runtime_error(
						(file_name.empty() ?
							"�w�肳�ꂽ�t�@�C��"
							: "[ " + file_name + " ] ")
						+ "��������܂���ł����B"
					);
			}
			catch (std::runtime_error& exception)
			{
				RuntimeErrorStop(exception);
			}
			assert(SUCCEEDED(hr) && "AssertLoadResult::�����s���̃G���[���������܂����B");
		}
	}
}
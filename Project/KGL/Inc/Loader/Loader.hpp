#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <string>
#include <filesystem>
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
							"指定されたファイル"
							: "[ " + file_name + " ] ")
						+ "が見つかりませんでした。"
					);
			}
			catch (std::runtime_error& exception)
			{
				RuntimeErrorStop(exception);
			}
			assert(SUCCEEDED(hr) && "AssertLoadResult::原因不明のエラーが発生しました。");
		}

		class Loader
		{
		protected:
			std::filesystem::path path;
		protected:
			static bool IsExtensiton(const std::filesystem::path& path, const std::string& extension) noexcept;
			static void CheckExtensiton(const std::filesystem::path& path, const std::string& extension) noexcept(false);
		protected:
			bool IsExtensiton(const std::string& extension) noexcept
			{
				return IsExtensiton(path, extension);
			}
			void CheckExtensiton(const std::string& extension) noexcept(false)
			{
				return CheckExtensiton(path, extension);
			}
			explicit Loader(const std::filesystem::path& path) noexcept	:
				// 文字列は正規化する
				path(path.lexically_normal())
			{

			}
			virtual ~Loader() = default;
		};
	}
}
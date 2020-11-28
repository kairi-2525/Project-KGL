#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <string>
#include <filesystem>
#include "../Helper/ThrowAssert.hpp"
#include "../Base/Model/StaticModel.hpp"

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
		private:
			std::filesystem::path m_path;
		protected:
			static bool IsExtensiton(const std::filesystem::path& path, const std::string& extension) noexcept;
			static void CheckExtensiton(const std::filesystem::path& path, const std::string& extension) noexcept(false);
		protected:
			bool IsExtensiton(const std::string& extension) noexcept
			{
				return IsExtensiton(m_path, extension);
			}
			void CheckExtensiton(const std::string& extension) noexcept(false)
			{
				return CheckExtensiton(m_path, extension);
			}
			explicit Loader(const std::filesystem::path& path) noexcept :
				// 文字列は正規化する
				m_path(path.lexically_normal())
			{

			}
			virtual ~Loader() = default;
		public:
			const std::filesystem::path& GetPath() const noexcept { return m_path; }
		};

		class StaticModelLoader : public Loader
		{
		private:
			std::shared_ptr<const S_MODEL::Materials> m_materials;
		protected:
			explicit StaticModelLoader(const std::filesystem::path& path) noexcept	:
				Loader(path)
			{
			}
			virtual ~StaticModelLoader() = default;
			void SetMaterials(std::shared_ptr<const S_MODEL::Materials> materials) noexcept { m_materials = materials; }
		public:
			std::shared_ptr<const S_MODEL::Materials> GetMaterials() const noexcept { return m_materials; }
		};
	}
}
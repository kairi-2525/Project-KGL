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
			bool loaded;
		protected:
			static bool IsExtension(const std::filesystem::path& path, const std::filesystem::path& extension) noexcept;
			static void CheckExtension(const std::filesystem::path& path, const std::filesystem::path& extension) noexcept(false);
		protected:
			bool IsExtensiton(const std::filesystem::path& extension) noexcept
			{
				return IsExtension(m_path, extension);
			}
			bool IsLoaded() const noexcept
			{
				return loaded;
			}
			void CheckExtensiton(const std::filesystem::path& extension) noexcept(false)
			{
				return CheckExtension(m_path, extension);
			}
			explicit Loader(const std::filesystem::path& path) noexcept :
				// 文字列は正規化する
				m_path(path.lexically_normal()), loaded(false)
			{

			}
			void ReplaceExtension(const std::filesystem::path& extension) noexcept;
			void Loaded() noexcept { loaded = true; }
			virtual ~Loader() = default;
		public:
			const std::filesystem::path& GetPath() const noexcept { return m_path; }
		};

		class StaticModelLoader : public Loader
		{
		public:
			static inline const std::filesystem::path EXTENSION = ".smodel";
		private:
			std::shared_ptr<const S_MODEL::Materials> m_materials;
		protected:
			explicit StaticModelLoader(const std::filesystem::path& path) noexcept;
			virtual ~StaticModelLoader() = default;
			void SetMaterials(std::shared_ptr<const S_MODEL::Materials> materials) noexcept { m_materials = materials; }
			bool
		public:
			bool Export(const std::filesystem::path& path);
			std::shared_ptr<const S_MODEL::Materials> GetMaterials() const noexcept { return m_materials; }
		};
	}
}
#pragma once

#include "../Base/OBJ.hpp"
#include "./Loader.hpp"
#include <fstream>

namespace KGL
{
	inline namespace LOADER
	{
		class OBJ_Loader : public StaticModelLoader
		{
		private:
			std::shared_ptr<const OBJ::Desc> m_desc;
		private:
			void LoadMTLFile(
				const std::filesystem::path& r_path,
				std::shared_ptr<OBJ::Desc> out_desc
			) noexcept(false);
			void LoadObjects(
				std::ifstream& ifs,
				std::shared_ptr<OBJ::Desc> out_desc,
				std::shared_ptr<OBJ::Object> out_objects
			) noexcept;
			bool LoadMaterials(
				std::ifstream& ifs,
				const std::filesystem::path& r_path,
				std::shared_ptr<OBJ::Desc> out_desc,
				std::shared_ptr<OBJ::Material> out_material,
				bool smooth_flg
			) noexcept(false);

			// StaticModel用のマテリアルに変換
			void ConvertMaterial() noexcept(false);
		public:
			explicit OBJ_Loader(const std::filesystem::path& path, bool fast_load = false) noexcept;
		};
	}
}
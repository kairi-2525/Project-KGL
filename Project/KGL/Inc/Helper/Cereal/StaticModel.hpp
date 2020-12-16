#pragma once

#include "./DirectXMath.hpp"
#include "./CerealHelper.hpp"
#include "../../Base/Model/StaticModel.hpp"
#include <Cereal/types/string.hpp>
#include <Cereal/types/unordered_map.hpp>
#include <Cereal/types/vector.hpp>
#include <Cereal/types/utility.hpp>

namespace KGL
{
	inline namespace BASE
	{
		namespace S_MODEL
		{
			enum class Version : std::uint32_t
			{
				V0
			};

			template<class Archive>
			void serialize(
				Archive& archive,
				Vertex& m,
				std::uint32_t const version
			) {
				auto ver = static_cast<Version>(version);
				switch (ver)
				{
					case Version::V0:
					{
						archive(
							m.position,
							m.uv,
							m.normal,
							m.tangent,
							m.bitangent
						);
						break;
					}
				}
			}
			template<class Archive>
			void serialize(
				Archive& archive,
				Textures& m,
				std::uint32_t const version
			) {
				auto ver = static_cast<Version>(version);
				switch (ver)
				{
					case Version::V0:
					{
						archive(
							m.ambient,
							m.diffuse,
							m.specular,
							m.specular_highlights,
							m.dissolve,
							m.bump,
							m.displacement,
							m.stencil_decal,
							m.reflections
						);
						break;
					}
				}
			}
			template<class Archive>
			void serialize(
				Archive& archive,
				Material::Parameter& m,
				std::uint32_t const version
			) {
				auto ver = static_cast<Version>(version);
				switch (ver)
				{
					case Version::V0:
					{
						archive(
							m.ambient_color,
							m.diffuse_color,
							m.specular_color,
							m.specular_weight,
							m.specular_flg,
							m.dissolve,
							m.refraction,
							m.smooth
						);
						m.pad0 = m.pad1 = 0.f;
						ZeroMemory(m.pad2, sizeof(m.pad2));
						ZeroMemory(m.pad3, sizeof(m.pad3));
						break;
					}
				}
			}
			template<class Archive>
			void serialize(
				Archive& archive,
				Material& m,
				std::uint32_t const version
			) {
				auto ver = static_cast<Version>(version);
				switch (ver)
				{
					case Version::V0:
					{
						archive(
							m.vertices,
							m.tex,
							m.param
						);
						break;
					}
				}
			}
		}
	}
}
CEREAL_CLASS_VERSION(KGL::BASE::S_MODEL::Vertex,
	static_cast<std::uint32_t>(KGL::BASE::S_MODEL::Version::V0));
CEREAL_CLASS_VERSION(KGL::BASE::S_MODEL::Textures,
	static_cast<std::uint32_t>(KGL::BASE::S_MODEL::Version::V0));
CEREAL_CLASS_VERSION(KGL::BASE::S_MODEL::Material::Parameter,
	static_cast<std::uint32_t>(KGL::BASE::S_MODEL::Version::V0));
CEREAL_CLASS_VERSION(KGL::BASE::S_MODEL::Material,
	static_cast<std::uint32_t>(KGL::BASE::S_MODEL::Version::V0));
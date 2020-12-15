#pragma once

#include "./DirectXMath.hpp"
#include "../../Base/Model/StaticModel.hpp"

namespace KGL
{
	inline namespace BASE
	{
		namespace S_MODEL
		{
			enum class Version : std::uint32_t
			{
				V_0
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
					case Version::V_0:
					{
						archive(m.position, m.uv, m.normal, m.tangent, m.bitangent);
						break;
					}
				}
			}
		}
	}
}
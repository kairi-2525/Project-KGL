#pragma once

#include "../../Loader/Loader.hpp"
#include "../ConstantBuffer.hpp"

namespace KGL
{
	inline namespace DX12
	{
		inline namespace _3D
		{
			class StaticModel
			{
			private:
				struct Material
				{
					std::shared_ptr<Resource<S_MODEL::Vertex>>	rs_vertices;
				};
			private:
				std::vector<Material>	m_materials;
			public:
				explicit StaticModel(
					ComPtrC<ID3D12Device> device,
					std::shared_ptr<const StaticModelLoader> loader
				) noexcept;
			};
		}
	}
}
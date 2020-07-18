#pragma once

#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")

#include <DirectXMath.h>
#include "../../Helper/ComPtr.hpp"
#include <array>

namespace KGL
{
	inline namespace DX12
	{
		inline namespace _3D
		{
			struct Board
			{
			public:
				struct Vertex
				{
					DirectX::XMFLOAT3 pos;
					DirectX::XMFLOAT3 normal;
					DirectX::XMFLOAT2 uv;
				};
				using Vertices = std::array<Vertex, 4>;
			private:
				ComPtr<ID3D12Resource>		m_vert_buff;
				D3D12_VERTEX_BUFFER_VIEW	m_vbv;
			public:
				Board(const ComPtrC<ID3D12Device>& device) noexcept;
				void Render(const ComPtr<ID3D12GraphicsCommandList>& cmd_list, UINT index_count = 1u) const noexcept;
			};
		}
	}
}
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
		inline namespace _2D
		{
				class Sprite
			{
			public:
				struct Vertex
				{
					DirectX::XMFLOAT3 position;
					DirectX::XMFLOAT2 uv;
				};
				using Vertices = std::array<Vertex, 4>;
			private:
				ComPtr<ID3D12Resource>		m_vert_buff;
				D3D12_VERTEX_BUFFER_VIEW	m_vbv;
			public:
				Sprite(const ComPtrC<ID3D12Device>& device) noexcept;
				HRESULT SetVertices(const Vertices& vertices) const noexcept;
				HRESULT GetVertices(Vertices* p_vertices) const noexcept;

				HRESULT SetPos(const RECT& rect, float screen_w, float screen_h) const noexcept;

				void Render(const ComPtr<ID3D12GraphicsCommandList>& cmd_list) const noexcept;
			};
		}
	}
}
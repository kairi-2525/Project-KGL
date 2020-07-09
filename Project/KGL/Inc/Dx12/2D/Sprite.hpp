#pragma once

#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")

#include <DirectXMath.h>
#include "../../Helper/ComPtr.hpp"

namespace KGL
{
	inline namespace DX12
	{
		class Sprite
		{
		public:
			struct Vertex
			{
				DirectX::XMFLOAT3 position;
				DirectX::XMFLOAT2 uv;
			};
		private:
			ComPtr<ID3D12Resource>		m_vert_buff;
			ComPtr<ID3D12Resource>		m_idx_buff;
			D3D12_VERTEX_BUFFER_VIEW	m_vbv;
		public:
			Sprite(const ComPtr<ID3D12Device>& device) noexcept;
			void Render(const ComPtr<ID3D12GraphicsCommandList>& cmd_list);
		};
	}
}
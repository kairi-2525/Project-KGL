#pragma once

#include <DirectXMath.h>
#include "../Helper/ComPtr.hpp"

#include "PMDModel.hpp"

namespace KGL
{
	inline namespace DX12
	{
		class PMD_Actor
		{
		protected:
			struct ConstantBuffers
			{
				DirectX::XMMATRIX wvp;
				DirectX::XMMATRIX world;
				DirectX::XMMATRIX view;
				DirectX::XMMATRIX proj;
				DirectX::XMMATRIX bones[512];
				DirectX::XMFLOAT3 eye;	// éãì_ç¿ïW
			};
			ConstantBuffers* m_map_buffers;
			ComPtr<ID3D12DescriptorHeap>	m_desc_heap;
			ComPtr<ID3D12Resource>			m_const_buff;
		public:
			explicit PMD_Actor(
				const ComPtr<ID3D12Device>& device,
				const std::vector<DirectX::XMMATRIX>& bones
			) noexcept;
			virtual ~PMD_Actor() = default;

			void SetViewProjection(DirectX::XMMATRIX view, DirectX::XMMATRIX proj)
			{ m_map_buffers->view = view; m_map_buffers->proj = proj; }
			void SetEye(const DirectX::XMFLOAT3& eye) { m_map_buffers->eye = eye; }
			void UpdateWVP();
			void Render(
				const ComPtr<ID3D12GraphicsCommandList>& cmd_list
			) const noexcept;
			ConstantBuffers* GetMappedBuffers() { return m_map_buffers; }
			void RecursiveMatrixMultiply(
				const PMD::BoneNode* node,
				const DirectX::XMMATRIX& mat
			) noexcept;
		};
	}
}
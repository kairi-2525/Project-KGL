#pragma once

#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")

namespace KGL
{
	inline namespace DX12
	{
		/*class PipelineStates
		{
		private:
			using States = std::map<ComPtr<ID3D12PipelineState>, D3D12_GRAPHICS_PIPELINE_STATE_DESC>;
		private:
			D3D12_GRAPHICS_PIPELINE_STATE_DESC	m_desc;
			ComPtr<ID3D12PipelineState>			m_state;
		public:
			PipelineStates() = default;
			PipelineStates(ComPtr<ID3D12Device> device, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc) noexcept
			{ SetDesc(device, desc); }
			const D3D12_GRAPHICS_PIPELINE_STATE_DESC& GetDesc() const noexcept { return m_desc; }
			HRESULT SetDesc(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc) noexcept { return SetDesc({}, desc); }
			HRESULT SetDesc(ComPtr<ID3D12Device> device, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc) noexcept;
		};*/
	}
}
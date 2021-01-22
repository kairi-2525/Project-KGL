#pragma once

#include "Signature.hpp"
#include <functional>

namespace KGL
{
	inline namespace DX12
	{
		namespace DXR
		{
			class BaseRenderer
			{
			public:
				struct Desc
				{
					SignatureList						signatures;
					std::map<std::string, std::string>	hit_groups;
					D3D12_RAYTRACING_SHADER_CONFIG		shader_config;

					UINT max_trace_recursion_depth;
				};
			private:
				ComPtr<ID3D12StateObject> m_state_object;
			protected:
				BaseRenderer() = default;
				HRESULT Create(
					ComPtrC<ID3D12Device5> device,
					const std::shared_ptr<DXC>& dxc,
					const Desc& desc
				) noexcept;
			public:
				explicit BaseRenderer(
					ComPtrC<ID3D12Device5> device,
					const std::shared_ptr<DXC>& dxc,
					const Desc& desc
				) noexcept
				{
					Create(device, dxc, desc);
				}
				void Set(ComPtrC<ID3D12GraphicsCommandList4> cmd_list) const noexcept;
			};
		}
	}
}
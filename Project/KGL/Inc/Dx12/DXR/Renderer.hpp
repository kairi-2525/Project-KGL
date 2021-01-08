#pragma once

#include "Signature.hpp"

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
					SignatureList signatures;
				};
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
			};
		}
	}
}
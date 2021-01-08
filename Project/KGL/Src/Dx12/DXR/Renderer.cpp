#include <Dx12/DXR/Renderer.hpp>

using namespace KGL;

HRESULT DXR::BaseRenderer::Create(
	ComPtrC<ID3D12Device5> device,
	const std::shared_ptr<DXC>& dxc,
	const Desc& desc
) noexcept
{
	HRESULT hr = S_OK;

	DXR::Signature signature(device, dxc, desc.signatures);

	return hr;
}
#include <Dx12/BlendState.hpp>
#include <Helper/ThrowAssert.hpp>
#include <DirectXTex/d3dx12.h>

using namespace KGL;

HRESULT BLEND::SetBlend(TYPE type, D3D12_BLEND_DESC* desc)
{
	RCHECK(!desc, "desc ‚ª nullptr", E_FAIL);

	switch (type)
	{
	case TYPE::DEFAULT:
	default:
		*desc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		break;
	}
	return S_OK;
}
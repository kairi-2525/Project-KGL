#include <Dx12/BlendState.hpp>
#include <Helper/ThrowAssert.hpp>
#include <DirectXTex/d3dx12.h>

using namespace KGL;

HRESULT BLEND::SetBlend(const TYPES& types, D3D12_BLEND_DESC* desc)
{
	RCHECK(!desc, "desc ‚ª nullptr", E_FAIL);

	constexpr size_t rt_num = 8;
	for (size_t i = 0u; i < rt_num; i++)
	{
		desc->RenderTarget[i].BlendEnable = TRUE;
		desc->RenderTarget[i].LogicOp = D3D12_LOGIC_OP_NOOP;
		desc->RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		switch (types[i])
		{
			case TYPE::ALPHA:
			{
				desc->RenderTarget[i].SrcBlend = D3D12_BLEND_SRC_ALPHA;
				desc->RenderTarget[i].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
				desc->RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
				desc->RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;
				desc->RenderTarget[i].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
				desc->RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
				break;
			}
			case TYPE::ADD:
			{
				desc->RenderTarget[i].SrcBlend = D3D12_BLEND_SRC_ALPHA;
				desc->RenderTarget[i].DestBlend = D3D12_BLEND_ONE;
				desc->RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
				desc->RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ZERO;
				desc->RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ONE;
				desc->RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
				break;
			}
			case TYPE::SUBTRACT:
			{
				desc->RenderTarget[i].SrcBlend = D3D12_BLEND_SRC_ALPHA;
				desc->RenderTarget[i].DestBlend = D3D12_BLEND_ONE;
				desc->RenderTarget[i].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
				desc->RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ZERO;
				desc->RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ONE;
				desc->RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
				break;
			}
			case TYPE::REPLEASE:
			{
				desc->RenderTarget[i].SrcBlend = D3D12_BLEND_SRC_ALPHA;
				desc->RenderTarget[i].DestBlend = D3D12_BLEND_ZERO;
				desc->RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
				desc->RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;
				desc->RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ZERO;
				desc->RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
				break;
			}
			case TYPE::MULTIPLY:
			{
				desc->RenderTarget[i].SrcBlend = D3D12_BLEND_ZERO;
				desc->RenderTarget[i].DestBlend = D3D12_BLEND_SRC_ALPHA;
				desc->RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
				desc->RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ZERO;
				desc->RenderTarget[i].DestBlendAlpha = D3D12_BLEND_SRC_ALPHA;
				desc->RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
				break;
			}
			case TYPE::LIGHTEN:
			{
				desc->RenderTarget[i].SrcBlend = D3D12_BLEND_ONE;
				desc->RenderTarget[i].DestBlend = D3D12_BLEND_ONE;
				desc->RenderTarget[i].BlendOp = D3D12_BLEND_OP_MAX;
				desc->RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;
				desc->RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ONE;
				desc->RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_MAX;
				break;
			}
			case TYPE::DARKEN:
			{
				desc->RenderTarget[i].SrcBlend = D3D12_BLEND_ONE;
				desc->RenderTarget[i].DestBlend = D3D12_BLEND_ONE;
				desc->RenderTarget[i].BlendOp = D3D12_BLEND_OP_MIN;
				desc->RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;
				desc->RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ONE;
				desc->RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_MIN;
				break;
			}
			case TYPE::SCREEN:
			{
				desc->RenderTarget[i].SrcBlend = D3D12_BLEND_SRC_ALPHA;
				desc->RenderTarget[i].DestBlend = D3D12_BLEND_INV_SRC_COLOR;
				desc->RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
				desc->RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;
				desc->RenderTarget[i].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
				desc->RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
				break;
			}
			default:
			{
				desc->RenderTarget[i].BlendEnable = FALSE;
				desc->RenderTarget[i].SrcBlend = D3D12_BLEND_ONE;
				desc->RenderTarget[i].DestBlend = D3D12_BLEND_ZERO;
				desc->RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
				desc->RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;
				desc->RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ZERO;
				desc->RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
				break;
			}
		}
	}
	return S_OK;
}
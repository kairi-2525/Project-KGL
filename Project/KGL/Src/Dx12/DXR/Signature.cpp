#include <Dx12/DXR/Signature.hpp>
#include <Loader/Loader.hpp>

using namespace KGL;

DXR::Signature::Signature(
	ComPtrC<ID3D12Device> device,
	const std::shared_ptr<DXC>& dxc,
	const SignatureList& signatures
) noexcept
{
	HRESULT hr;
	try
	{
		for (const auto& it : signatures)
		{
			const auto& desc = it.second;
			auto& data = m_data[it.first];

			// シェーダーをロード
			const auto& shader = desc.shader;
			hr = Load(dxc, shader, &data.shader);
			RCHECK_STR(FAILED(hr), "[" + shader.hlsl.string() + "] の生成に失敗", "DXRシェーダーの生成に失敗");
			
			// root signature を作成
			D3D12_ROOT_SIGNATURE_DESC root_desc = {};
			root_desc.NumParameters = SCAST<UINT>(desc.root_params.size());
			root_desc.pParameters = desc.root_params.data();
			root_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

			ComPtr<ID3DBlob> sig_blob, error_blob;
			hr = D3D12SerializeRootSignature(
				&root_desc, D3D_ROOT_SIGNATURE_VERSION_1_0,
				sig_blob.GetAddressOf(),
				error_blob.GetAddressOf());
			RCHECK_STR(FAILED(hr), "[" + it.first + "] のRootSignatureSerializeに失敗", "DXR用RootSignatureのSerializeに失敗");
			
			hr = device->CreateRootSignature(
				0, sig_blob->GetBufferPointer(), sig_blob->GetBufferSize(),
				IID_PPV_ARGS(data.rs.ReleaseAndGetAddressOf()));
			RCHECK_STR(FAILED(hr), "[" + it.first + "] のRootSignature生成に失敗", "DXR用RootSignatureの生成に失敗");
		}
	}
	catch (std::runtime_error& exception)
	{
		RuntimeErrorStop(exception);
	}
}
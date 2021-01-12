#include <Dx12/DXR/Signature.hpp>
#include <Loader/Loader.hpp>
#include <Helper/Convert.hpp>

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

			// �V�F�[�_�[�����[�h
			const auto& shader = desc.shader;
			// SHADER::Desc�ɕϊ�
			SHADER::Desc shader_desc{};
			shader_desc.entry_point = "";
			shader_desc.hlsl = shader.hlsl;
			shader_desc.version = shader.version;

			// �R���p�C�������s���Aentry_points��ۑ�
			hr = Load(dxc, shader_desc, &data.shader);
			RCHECK_STR(FAILED(hr), "[" + shader.hlsl.string() + "] �̐����Ɏ��s", "DXR�V�F�[�_�[�̐����Ɏ��s");
			const size_t ep_size = shader.entry_points.size();
			data.entry_points.resize(ep_size);
			for (size_t i = 0u; i < ep_size; i++)
			{
				data.entry_points[i] = CONVERT::MultiToWide(shader.entry_points[i]);
			}

			// root signature ���쐬
			D3D12_ROOT_SIGNATURE_DESC root_desc = {};
			root_desc.NumParameters = SCAST<UINT>(desc.root_params.size());
			root_desc.pParameters = desc.root_params.data();
			root_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

			ComPtr<ID3DBlob> sig_blob, error_blob;
			hr = D3D12SerializeRootSignature(
				&root_desc, D3D_ROOT_SIGNATURE_VERSION_1_0,
				sig_blob.GetAddressOf(),
				error_blob.GetAddressOf());
			RCHECK_STR(FAILED(hr), "[" + it.first + "] ��RootSignatureSerialize�Ɏ��s", "DXR�pRootSignature��Serialize�Ɏ��s");
			
			hr = device->CreateRootSignature(
				0, sig_blob->GetBufferPointer(), sig_blob->GetBufferSize(),
				IID_PPV_ARGS(data.rs.ReleaseAndGetAddressOf()));
			RCHECK_STR(FAILED(hr), "[" + it.first + "] ��RootSignature�����Ɏ��s", "DXR�pRootSignature�̐����Ɏ��s");
		}
	}
	catch (std::runtime_error& exception)
	{
		RuntimeErrorStop(exception);
	}
}

DXR::DummySignature::DummySignature(
	ComPtrC<ID3D12Device> device,
	D3D12_ROOT_SIGNATURE_FLAGS flags) noexcept
{
	HRESULT hr = S_OK;

	D3D12_ROOT_SIGNATURE_DESC root_desc = {};
	root_desc.NumParameters = 0;
	root_desc.pParameters = nullptr;
	root_desc.Flags = flags;

	ComPtr<ID3DBlob> sig_blob, error_blob;

	hr = D3D12SerializeRootSignature(
		&root_desc, D3D_ROOT_SIGNATURE_VERSION_1,
		sig_blob.GetAddressOf(),
		error_blob.GetAddressOf());
	RCHECK_STR(FAILED(hr), "[DummySignature] ��RootSignatureSerialize�Ɏ��s", "DXR�pRootSignature��Serialize�Ɏ��s");

	hr = device->CreateRootSignature(
		0, sig_blob->GetBufferPointer(), sig_blob->GetBufferSize(),
		IID_PPV_ARGS(m_rs.ReleaseAndGetAddressOf()));
	RCHECK_STR(FAILED(hr), "[DummySignature] ��RootSignature�����Ɏ��s", "DXR�pRootSignature�̐����Ɏ��s");
}
#include <Dx12/PMDModel.hpp>
#include <DirectXTex/d3dx12.h>
#include <Helper/ThrowAssert.hpp>
#include <Helper/Cast.hpp>
#include <Dx12/SetName.hpp>
#include <array>

using namespace KGL;

PMD_Model::PMD_Model(ComPtr<ID3D12Device> device,
	const PMD::Desc& desc, TextureManager* mgr) noexcept
	: m_vb_view{}, m_ib_view{}
{
	if (!device)
	{
		assert(!"デバイスにNULLが渡されました。");
		return;
	}

	if (!mgr) mgr = &m_tex_mgr;

	m_tex_white = std::make_unique<Texture>(device, 0xff, 0xff, 0xff, 0xff, mgr);
	m_tex_black = std::make_unique<Texture>(device, 0x00, 0x00, 0x00, 0xff, mgr);
	m_tex_gradation = std::make_unique<Texture>(device, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 256, mgr);

	HRESULT hr = S_OK;

	hr = CreateVertexBuffers(device, desc.vertices);
	RCHECK(FAILED(hr), "バーテックスバッファの作成に失敗");
	SetName(m_vert_buff, RCAST<INT_PTR>(this), desc.path.wstring(), L"Vertex");

	hr = CreateIndexBuffers(device, desc.indices);
	RCHECK(FAILED(hr), "インデックスバッファの作成に失敗");
	SetName(m_idx_buff, RCAST<INT_PTR>(this), desc.path.wstring(), L"Index");

	hr = CreateMaterialBuffers(device, desc.materials);
	RCHECK(FAILED(hr), "マテリアルバッファの作成に失敗");
	SetName(m_idx_buff, RCAST<INT_PTR>(this), desc.path.wstring(), L"Material");
}

HRESULT PMD_Model::CreateVertexBuffers(ComPtr<ID3D12Device> device, const std::vector<UCHAR>& vert) noexcept
{
	HRESULT hr = S_OK;

	hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vert.size()),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_vert_buff.ReleaseAndGetAddressOf())
	);

	UCHAR* vert_map = nullptr;
	// 第二引数がnullptrだと全範囲を指定する
	hr = m_vert_buff->Map(0, nullptr, (void**)&vert_map);
	RCHECK(FAILED(hr), "バーテックスバッファのMapに失敗", hr);
	std::copy(std::cbegin(vert), std::cend(vert), vert_map);
	m_vert_buff->Unmap(0, nullptr);

	m_vb_view.BufferLocation = m_vert_buff->GetGPUVirtualAddress();	// バッファーの仮想アドレス。
	m_vb_view.SizeInBytes = SCAST<UINT>(vert.size());							// 全バイト数
	m_vb_view.StrideInBytes = PMD::VERTEX_SIZE;						// １頂点あたりのバイト数

	return hr;
}

HRESULT PMD_Model::CreateIndexBuffers(ComPtr<ID3D12Device> device, const std::vector<USHORT>& idx) noexcept
{
	HRESULT hr = S_OK;

	hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(idx.size() * sizeof(idx[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_idx_buff.ReleaseAndGetAddressOf())
	);

	// インデックスデータをバッファにコピー
	unsigned short* mapped_idx = nullptr;
	hr = m_idx_buff->Map(0, nullptr, (void**)&mapped_idx);
	RCHECK(FAILED(hr), "インデックスバッファのMapに失敗", hr);
	std::copy(std::cbegin(idx), std::cend(idx), mapped_idx);
	m_idx_buff->Unmap(0, nullptr);

	// インデックスバッファビューを作成
	m_ib_view.BufferLocation = m_idx_buff->GetGPUVirtualAddress();
	m_ib_view.Format = DXGI_FORMAT_R16_UINT;
	m_ib_view.SizeInBytes = SCAST<UINT>(idx.size() * sizeof(idx[0]));

	return hr;
}

HRESULT PMD_Model::CreateMaterialBuffers(ComPtr<ID3D12Device> device, const std::vector<PMD::Material>& mtr) noexcept
{
	HRESULT hr = S_OK;
	const size_t material_size = mtr.size();
	std::vector<MODEL::MaterialForHLSL> hlsl_materials(mtr.size());

	const size_t material_buffer_size = (sizeof(MODEL::MaterialForHLSL) + 0xff) & ~0xff;

	for (size_t i = 0u; i < material_size; i++)
	{
		hlsl_materials[i].deffuse = mtr.at(i).diffuse;
		hlsl_materials[i].alpha = mtr.at(i).alpha;
		hlsl_materials[i].specular = mtr.at(i).specular;
		hlsl_materials[i].specularity = mtr.at(i).specularity;
		hlsl_materials[i].ambient = mtr.at(i).ambient;
	}

	hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(material_buffer_size * material_size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_mtr_buff.ReleaseAndGetAddressOf())
	);

	// マテリアルデータをバッファにコピー
	CHAR* map_material = nullptr;
	hr = m_mtr_buff->Map(0, nullptr, (void**)&map_material);
	RCHECK(FAILED(hr), "マテリアルバッファのMapに失敗", hr);

	for (auto& material : hlsl_materials)
	{
		*((MODEL::MaterialForHLSL*)map_material) = material;
		map_material += material_buffer_size;	// 次のアライメント位置まで進む
	}
	m_mtr_buff->Unmap(0, nullptr);

	return hr;
}
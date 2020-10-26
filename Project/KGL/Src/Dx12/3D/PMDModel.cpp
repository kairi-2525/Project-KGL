#include <Dx12/3D/PMDModel.hpp>
#include <DirectXTex/d3dx12.h>
#include <Helper/ThrowAssert.hpp>
#include <Helper/Cast.hpp>
#include <Dx12/SetName.hpp>
#include <array>

using namespace KGL;

PMD_Model::PMD_Model(
	const ComPtr<ID3D12Device>& device,
	const std::shared_ptr<const PMD::Desc>& desc,
	const std::filesystem::path& toon_folder, TextureManager* mgr
) noexcept
	: m_vb_view{}, m_ib_view{}, m_material_num(desc->materials.size()), m_desc(desc)
{
	if (!device)
	{
		assert(!"�f�o�C�X��NULL���n����܂����B");
		return;
	}

	if (!mgr) mgr = &m_tex_mgr;

	HRESULT hr = S_OK;

	hr = CreateVertexBuffers(device, m_desc->vertices);
	RCHECK(FAILED(hr), "�o�[�e�b�N�X�o�b�t�@�̍쐬�Ɏ��s");
	SetName(m_vert_buff, RCAST<INT_PTR>(this), m_desc->path.wstring(), L"Vertex");

	hr = CreateIndexBuffers(device, m_desc->indices);
	RCHECK(FAILED(hr), "�C���f�b�N�X�o�b�t�@�̍쐬�Ɏ��s");
	SetName(m_idx_buff, RCAST<INT_PTR>(this), m_desc->path.wstring(), L"Index");

	hr = CreateMaterialBuffers(device, m_desc->materials);
	RCHECK(FAILED(hr), "�}�e���A���o�b�t�@�̍쐬�Ɏ��s");
	SetName(m_idx_buff, RCAST<INT_PTR>(this), m_desc->path.wstring(), L"Material");

	hr = CreateTextureBuffers(device, m_desc->materials, m_desc->path, toon_folder, m_desc->toon_tex_table, mgr);
	RCHECK(FAILED(hr), "�e�N�X�`���o�b�t�@�[�̍쐬�Ɏ��s");

	hr = CreateMaterialHeap(device);
	RCHECK(FAILED(hr), "�e�N�X�`���o�b�t�@�[�̍쐬�Ɏ��s");

	hr = CreateBoneMatrix(desc->bone_node_table);
	RCHECK(FAILED(hr), "�{�[���p��Matrix�̍쐬�Ɏ��s");
}

HRESULT PMD_Model::CreateVertexBuffers(ComPtr<ID3D12Device> device, const std::vector<PMD::Vertex>& vert) noexcept
{
	HRESULT hr = S_OK;

	hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vert.size() * PMD::VERTEX_SIZE),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_vert_buff.ReleaseAndGetAddressOf())
	);

	PMD::Vertex* vert_map = nullptr;
	// ��������nullptr���ƑS�͈͂��w�肷��
	hr = m_vert_buff->Map(0, nullptr, (void**)&vert_map);
	RCHECK(FAILED(hr), "�o�[�e�b�N�X�o�b�t�@��Map�Ɏ��s", hr);
	std::copy(std::cbegin(vert), std::cend(vert), vert_map);
	m_vert_buff->Unmap(0, nullptr);

	m_vb_view.BufferLocation = m_vert_buff->GetGPUVirtualAddress();	// �o�b�t�@�[�̉��z�A�h���X�B
	m_vb_view.SizeInBytes = SCAST<UINT>(vert.size() * PMD::VERTEX_SIZE);	// �S�o�C�g��
	m_vb_view.StrideInBytes = PMD::VERTEX_SIZE;						// �P���_������̃o�C�g��

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

	// �C���f�b�N�X�f�[�^���o�b�t�@�ɃR�s�[
	unsigned short* mapped_idx = nullptr;
	hr = m_idx_buff->Map(0, nullptr, (void**)&mapped_idx);
	RCHECK(FAILED(hr), "�C���f�b�N�X�o�b�t�@��Map�Ɏ��s", hr);
	std::copy(std::cbegin(idx), std::cend(idx), mapped_idx);
	m_idx_buff->Unmap(0, nullptr);

	// �C���f�b�N�X�o�b�t�@�r���[���쐬
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

	m_index_counts.resize(m_material_num);
	m_index_count_total = 0;
	for (size_t i = 0u; i < material_size; i++)
	{
		m_index_counts[i] = mtr.at(i).indices_num;
		m_index_count_total += m_index_counts[i];

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

	// �}�e���A���f�[�^���o�b�t�@�ɃR�s�[
	CHAR* map_material = nullptr;
	hr = m_mtr_buff->Map(0, nullptr, (void**)&map_material);
	RCHECK(FAILED(hr), "�}�e���A���o�b�t�@��Map�Ɏ��s", hr);

	for (auto& material : hlsl_materials)
	{
		*((MODEL::MaterialForHLSL*)map_material) = material;
		map_material += material_buffer_size;	// ���̃A���C�����g�ʒu�܂Ői��
	}
	m_mtr_buff->Unmap(0, nullptr);

	return hr;
}

HRESULT PMD_Model::CreateTextureBuffers(ComPtr<ID3D12Device> device, const std::vector<PMD::Material>& mtr,
	const std::filesystem::path& path, const std::filesystem::path& toon_folder,
	const PMD::ToonTextureTable& toon_table,
	TextureManager* mgr) noexcept
{
	HRESULT hr = S_OK;
	const size_t material_size = mtr.size();

	// �e�N�X�`�����g�p���Ȃ��ꍇ�͂�����n���Ă���
	auto tex_white = std::make_unique<Texture>(device, 0xff, 0xff, 0xff, 0xff, mgr);
	auto tex_black = std::make_unique<Texture>(device, 0x00, 0x00, 0x00, 0xff, mgr);
	auto tex_gradation = std::make_unique<Texture>(device, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 256, mgr);

	m_textures.resize(material_size);

	auto model_folder = path;
	const bool has_toon_file = !toon_folder.empty();

	// toon_folder�̖����� "/"�����Ă��Ȃ��ꍇ�t������
	auto use_toon_folder = toon_folder;
	if (has_toon_file)
	{
		use_toon_folder.make_preferred();
		auto str = use_toon_folder.string();
		auto pos = str.rfind('\\');
		if (pos != std::string::npos)
		{
			if (str.begin() + pos != str.end() - 1)
			{
				use_toon_folder += '\\';
			}
		}
		else use_toon_folder += '\\';
	}
	model_folder.remove_filename();

	// �}�e���A�����ƂɃe�N�X�`���o�b�t�@�[���쐬���Ă���
	for (size_t i = 0u; i < material_size; i++)
	{
		auto& tex = m_textures[i];
		auto& material = mtr[i];
		if (has_toon_file)
		{
			std::string toon_file_name;
			if (toon_table.empty())
			{
				char toon_file_namec[16];
				sprintf_s(
					toon_file_namec,
					std::size(toon_file_namec),
					"toon%02d.bmp",
					material.toon_idx + 1
				);
				toon_file_name = toon_file_namec;
			}
			else
			{
				auto citr = toon_table.find(material.toon_idx);
				if (citr != toon_table.cend())
				{
					toon_file_name = citr->second.string();
				}
			}
			//KGLDebugOutPutString("[MTR " + std::to_string(i) + "] : " + std::to_string(material.toon_idx));

			tex.toon_buff = std::make_unique<Texture>();
			if (toon_file_name.empty())
			{
				*tex.toon_buff = *tex_gradation;
			}
			else
			{
				hr = tex.toon_buff->Create(device, use_toon_folder.string() + toon_file_name, 1u, mgr);
				if (!IsFound(hr))
				{
					hr = tex.toon_buff->Create(device, model_folder.string() + toon_file_name, 1u, mgr);
					if (!IsFound(hr))
					{
						*tex.toon_buff = *tex_gradation;
						hr = S_OK;
					}
				}
				RCHECK(FAILED(hr), "�g�[���o�b�t�@�̍쐬�Ɏ��s", hr);
			}
		}
		else
		{
			tex.toon_buff = std::make_unique<Texture>();
			*tex.toon_buff = *tex_gradation;
		}

		const std::filesystem::path diffuse_tex_path = material.tex_file_Path;
		if (!diffuse_tex_path.empty())
		{
			std::pair<std::filesystem::path, std::filesystem::path> tex_files;
			std::pair<std::string, std::string> extensions;
			auto star_pos = diffuse_tex_path.string().rfind('*');
			if (star_pos != std::string::npos)
			{
				tex_files.first = diffuse_tex_path.string().substr(0, star_pos);
				auto& etsf = extensions.first;
				etsf = tex_files.first.extension().string();
				std::transform(etsf.begin(), etsf.end(), etsf.begin(), static_cast<int (*)(int)>(&std::toupper));

				tex_files.second = diffuse_tex_path.string().substr(star_pos + 1);
				auto& etss = extensions.second;
				etss = tex_files.second.extension().string();
				std::transform(etss.begin(), etss.end(), etss.begin(), static_cast<int (*)(int)>(&std::toupper));
			}
			else
			{
				tex_files.first = diffuse_tex_path;
				auto& etsf = extensions.first;
				etsf = tex_files.first.extension().string();
				std::transform(etsf.begin(), etsf.end(), etsf.begin(), static_cast<int (*)(int)>(&std::toupper));
			}

			if (!tex_files.first.empty())
				tex_files.first = model_folder.string() + tex_files.first.string();
			if (!tex_files.second.empty())
				tex_files.second = model_folder.string() + tex_files.second.string();

			if (extensions.first == ".SPH")
				tex.sph_buff = std::make_unique<Texture>(device, tex_files.first, 1u, mgr);
			else if (extensions.first == ".SPA")
				tex.spa_buff = std::make_unique<Texture>(device, tex_files.first, 1u, mgr);
			else if (!extensions.first.empty())
				tex.diffuse_buff = std::make_unique<Texture>(device, tex_files.first, 1u, mgr);

			if (extensions.second == ".SPH")
				tex.sph_buff = std::make_unique<Texture>(device, tex_files.second, 1u, mgr);
			else if (extensions.second == ".SPA")
				tex.spa_buff = std::make_unique<Texture>(device, tex_files.second, 1u, mgr);
			else if (!extensions.second.empty())
				tex.diffuse_buff = std::make_unique<Texture>(device, tex_files.second, 1u, mgr);
		}
		
		if (!tex.sph_buff)
		{
			tex.sph_buff = std::make_unique<Texture>();
			*tex.sph_buff = *tex_white;
		}
		if (!tex.spa_buff)
		{
			tex.spa_buff = std::make_unique<Texture>();
			*tex.spa_buff = *tex_black;
		}
		if (!tex.diffuse_buff)
		{
			tex.diffuse_buff = std::make_unique<Texture>();
			*tex.diffuse_buff = *tex_white;
		}
	}
	return hr;
}

HRESULT PMD_Model::CreateMaterialHeap(ComPtr<ID3D12Device> device) noexcept
{
	HRESULT hr = S_OK;

	// �}�e���A���q�[�v�̍쐬
	{
		D3D12_DESCRIPTOR_HEAP_DESC mat_heap_desc = {};
		mat_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		mat_heap_desc.NodeMask = 0;
		mat_heap_desc.NumDescriptors = SCAST<UINT>(m_material_num * 5); // �}�e���A�� + SRV + �X�t�B�A�}�b�v�pSRV x2 + ToonSRV
		mat_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

		hr = device->CreateDescriptorHeap(
			&mat_heap_desc, IID_PPV_ARGS(m_mtr_heap.ReleaseAndGetAddressOf())
		);
		RCHECK(FAILED(hr), "CreateDescriptorHeap�Ɏ��s", hr);
	}

	const auto cbv_increment_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MipLevels = 1;

	D3D12_CONSTANT_BUFFER_VIEW_DESC mat_cbv_desc = {};
	mat_cbv_desc.BufferLocation = m_mtr_buff->GetGPUVirtualAddress();
	mat_cbv_desc.SizeInBytes = BUFFER_SIZE;

	auto heap_handle = m_mtr_heap->GetCPUDescriptorHandleForHeapStart();

	for (INT i = 0; i < m_material_num; i++)
	{
		auto& tex = m_textures[i];

		// Material
		device->CreateConstantBufferView(
			&mat_cbv_desc, heap_handle
		);
		heap_handle.ptr += cbv_increment_size;
		mat_cbv_desc.BufferLocation += mat_cbv_desc.SizeInBytes;

		// Diffuse
		srv_desc.Format = tex.diffuse_buff->Data()->GetDesc().Format;
		device->CreateShaderResourceView(
			tex.diffuse_buff->Data().Get(),
			&srv_desc,
			heap_handle
		);
		heap_handle.ptr += cbv_increment_size;

		// Sph
		srv_desc.Format = tex.sph_buff->Data()->GetDesc().Format;
		device->CreateShaderResourceView(
			tex.sph_buff->Data().Get(),
			&srv_desc,
			heap_handle
		);
		heap_handle.ptr += cbv_increment_size;

		// Spa
		srv_desc.Format = tex.spa_buff->Data()->GetDesc().Format;
		device->CreateShaderResourceView(
			tex.spa_buff->Data().Get(),
			&srv_desc,
			heap_handle
		);
		heap_handle.ptr += cbv_increment_size;

		// Toon
		srv_desc.Format = tex.toon_buff->Data()->GetDesc().Format;
		device->CreateShaderResourceView(
			tex.toon_buff->Data().Get(),
			&srv_desc,
			heap_handle
		);
		heap_handle.ptr += cbv_increment_size;
	}
	return hr;
}

HRESULT PMD_Model::CreateBoneMatrix(const PMD::BoneTable& bone_table) noexcept
{
	// ������
	m_bone_matrices.resize(bone_table.size());
	std::fill(
		m_bone_matrices.begin(),
		m_bone_matrices.end(),
		DirectX::XMMatrixIdentity()
	);

	return S_OK;
}

HRESULT PMD_Model::Render(
	const ComPtr<ID3D12Device>& device,
	const ComPtr<ID3D12GraphicsCommandList>& cmd_list,
	UINT instance_count
) const noexcept
{
	cmd_list->IASetVertexBuffers(0, 1, &m_vb_view);
	cmd_list->IASetIndexBuffer(&m_ib_view);
	cmd_list->SetDescriptorHeaps(1, m_mtr_heap.GetAddressOf());

	UINT index_offset = 0u;
	auto heap_handle = m_mtr_heap->GetGPUDescriptorHandleForHeapStart();
	const auto cbv_increment_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
	for (size_t i = 0u; i < m_material_num; i++)
	{
		const auto index_count = m_index_counts[i];
		cmd_list->SetGraphicsRootDescriptorTable(2, heap_handle);
		cmd_list->DrawIndexedInstanced(index_count, instance_count, index_offset, 0, 0);

		heap_handle.ptr += cbv_increment_size;
		index_offset += index_count;
	}

	return S_OK;
}

HRESULT PMD_Model::NonMaterialRender(
	const ComPtr<ID3D12Device>& device,
	const ComPtr<ID3D12GraphicsCommandList>& cmd_list,
	UINT instance_count
) const noexcept
{
	cmd_list->IASetVertexBuffers(0, 1, &m_vb_view);
	cmd_list->IASetIndexBuffer(&m_ib_view);

	cmd_list->DrawIndexedInstanced(m_index_count_total, instance_count, 0, 0, 0);

	return S_OK;
}
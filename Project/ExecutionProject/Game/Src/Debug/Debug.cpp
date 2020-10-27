#include "../../Hrd/Debug.hpp"
#include <imgui.h>
#include <DirectXTex/d3dx12.h>

DebugManager::DebugManager(ComPtrC<ID3D12Device> device, std::shared_ptr<KGL::BASE::DXC> dxc,
	DXGI_SAMPLE_DESC max_sample_desc)
{
	s_obj_wire = false;
	s_obj_changed = false;
	s_obj_vertices_offset = 0u;
	render_flg = false;

	s_obj_cbv_descmgr = std::make_shared<KGL::DescriptorManager>(device, 2u);
	s_obj_tc_handle = s_obj_cbv_descmgr->Alloc();
	s_obj_sc_handle = s_obj_cbv_descmgr->Alloc();
	s_obj_tc_resource = std::make_shared<KGL::Resource<TransformConstants>>(device, 1u);
	s_obj_sc_resource = std::make_shared<KGL::Resource<ShadingConstants>>(device, 1u);
	s_obj_vertex_resource = std::make_shared<KGL::Resource<Vertex>>(device, 256u);
	s_objects.reserve(s_obj_vertex_resource->Size());

	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
		cbv_desc.BufferLocation = s_obj_tc_resource->Data()->GetGPUVirtualAddress();
		cbv_desc.SizeInBytes = SCAST<UINT>(s_obj_tc_resource->SizeInBytes());
		device->CreateConstantBufferView(&cbv_desc, s_obj_tc_handle.Cpu());
	}
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
		cbv_desc.BufferLocation = s_obj_sc_resource->Data()->GetGPUVirtualAddress();
		cbv_desc.SizeInBytes = SCAST<UINT>(s_obj_sc_resource->SizeInBytes());
		device->CreateConstantBufferView(&cbv_desc, s_obj_sc_handle.Cpu());
	}

	s_obj_vertex_view.BufferLocation = s_obj_vertex_resource->Data()->GetGPUVirtualAddress();
	s_obj_vertex_view.SizeInBytes = SCAST<UINT>(s_obj_vertex_resource->SizeInBytes());
	s_obj_vertex_view.StrideInBytes = sizeof(Vertex);

	{
		auto renderer_desc = KGL::_3D::Renderer::DEFAULT_DESC;
		renderer_desc.input_layouts.clear();
		renderer_desc.input_layouts.push_back({
			"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		/*renderer_desc.input_layouts.push_back({
			"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });*/
		renderer_desc.input_layouts.push_back({
			"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });

		std::vector<D3D12_DESCRIPTOR_RANGE> drs_cbv =
		{
			{ D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1u, 0u, 0u, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
			{ D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1u, 1u, 0u, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND }
		};
		std::vector<D3D12_DESCRIPTOR_RANGE> drs_srv =
		{
			{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, 0u, 0u, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
			{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, 1u, 0u, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
			{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, 2u, 0u, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
			{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, 3u, 0u, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
			{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, 4u, 0u, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
			{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, 5u, 0u, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
			{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, 6u, 0u, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
		};

		std::vector<D3D12_ROOT_PARAMETER> root_params =
		{
			{ D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
			{ { SCAST<UINT>(drs_cbv.size()), drs_cbv.data() } },
			D3D12_SHADER_VISIBILITY_ALL },
			{ D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
			{ { SCAST<UINT>(drs_srv.size()), drs_srv.data() } },
			D3D12_SHADER_VISIBILITY_PIXEL },
		};
		renderer_desc.root_params = root_params;

		std::vector<D3D12_STATIC_SAMPLER_DESC> static_samplers =
		{
			CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_ANISOTROPIC),
			CD3DX12_STATIC_SAMPLER_DESC(1, D3D12_FILTER_MIN_MAG_MIP_LINEAR)
		};
		static_samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		static_samplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		static_samplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		static_samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		renderer_desc.static_samplers = static_samplers;

		renderer_desc.vs_desc.hlsl = "./HLSL/3D/StaticObject_vs.hlsl";
		renderer_desc.gs_desc.hlsl = "./HLSL/3D/StaticObject_gs.hlsl";
		renderer_desc.ps_desc.hlsl = "./HLSL/3D/StaticObject_ps.hlsl";

		renderer_desc.rastarizer_desc.FrontCounterClockwise = TRUE;
		renderer_desc.rastarizer_desc.CullMode = D3D12_CULL_MODE_BACK;
		s_obj_renderers.push_back(std::make_shared<KGL::_3D::Renderer>(device, dxc, renderer_desc));
		renderer_desc.rastarizer_desc.MultisampleEnable = TRUE;
		// MSAA用
		for (; renderer_desc.other_desc.sample_desc.Count < max_sample_desc.Count;)
		{
			renderer_desc.other_desc.sample_desc.Count *= 2;
			s_obj_renderers.push_back(std::make_shared<KGL::_3D::Renderer>(device, dxc, renderer_desc));
		}

		renderer_desc.rastarizer_desc.MultisampleEnable = FALSE;
		renderer_desc.other_desc.sample_desc.Count = 1u;
		renderer_desc.rastarizer_desc.FillMode = D3D12_FILL_MODE_WIREFRAME;
		s_obj_wire_renderers.push_back(std::make_shared<KGL::_3D::Renderer>(device, dxc, renderer_desc));
		// MSAA用
		for (; renderer_desc.other_desc.sample_desc.Count < max_sample_desc.Count;)
		{
			renderer_desc.other_desc.sample_desc.Count *= 2;
			s_obj_wire_renderers.push_back(std::make_shared<KGL::_3D::Renderer>(device, dxc, renderer_desc));
		}
	}

	{	// テクスチャ
		s_obj_srv_descmgr = std::make_shared<KGL::DescriptorManager>(device, 7u);

		s_obj_albedo.tex = std::make_shared<KGL::Texture>(device, "./Assets/Textures/Cubes/AwsomeBump/Albedo.png");
		s_obj_normal.tex = std::make_shared<KGL::Texture>(device, "./Assets/Textures/Cubes/AwsomeBump/Normal.png");
		s_obj_metalness.tex = std::make_shared<KGL::Texture>(device, "./Assets/Textures/Cubes/AwsomeBump/Metaric.png");
		s_obj_roughness.tex = std::make_shared<KGL::Texture>(device, "./Assets/Textures/Cubes/AwsomeBump/Roughness.png");
		s_obj_specular.tex = std::make_shared<KGL::Texture>(device, "./Assets/Textures/Cubes/AwsomeBump/Specular.png");
		//s_obj_specular.tex = std::make_shared<KGL::Texture>(device);
		s_obj_irradiance.tex = std::make_shared<KGL::Texture>(device);
		s_obj_specular_brdf.tex = std::make_shared<KGL::Texture>(device);

		white_texture = std::make_shared<KGL::Texture>(device);
		black_texture = std::make_shared<KGL::Texture>(device, 0, 0, 0);

		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MipLevels = 1;
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		Texture* textures[] = 
		{ 
			&s_obj_albedo, &s_obj_normal, &s_obj_metalness, &s_obj_roughness,
			&s_obj_specular, &s_obj_irradiance, &s_obj_specular_brdf 
		};
		for (auto* tex : textures)
		{
			tex->handle = s_obj_srv_descmgr->Alloc();
			srv_desc.Format = tex->tex->Data()->GetDesc().Format;
			device->CreateShaderResourceView(tex->tex->Data().Get(), &srv_desc, tex->handle.Cpu());
		}
	}
}

void DebugManager::AddStaticObjects(const std::vector<std::shared_ptr<Object>>& objects)
{
	if (!objects.empty())
	{
		s_obj_changed = true;
		s_objects.reserve(s_objects.size() + objects.size());
		std::copy(objects.begin(), objects.end(), std::back_inserter(s_objects));
	}
}

void DebugManager::ClearStaticObjects()
{
	auto* mapped_vertices = s_obj_vertex_resource->Map(0, &CD3DX12_RANGE(0u, 0u));
	ZeroMemory(mapped_vertices, s_obj_vertex_resource->SizeInBytes());
	s_obj_vertex_resource->Unmap(0, &CD3DX12_RANGE(0u, 0u));
}

HRESULT DebugManager::UpdateStaticObjects()
{
	HRESULT hr = S_OK;

	size_t vert_count = 0u;
	for (const auto& obj : s_objects)
	{
		vert_count += obj->GetVertexCount();
	}

	// リソースのサイズが足りない場合
	if (s_obj_vertex_resource->Size() < vert_count)
	{
		ComPtr<ID3D12Device> device;
		hr = s_obj_vertex_resource->Data()->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
		RCHECK_HR(hr, "DebugManager::UpdateStaticObjects で GetDevice に失敗");
		s_obj_vertex_resource = std::make_shared<KGL::Resource<Vertex>>(device, vert_count * 2u);

		s_obj_vertex_view.BufferLocation = s_obj_vertex_resource->Data()->GetGPUVirtualAddress();
		s_obj_vertex_view.SizeInBytes = SCAST<UINT>(s_obj_vertex_resource->SizeInBytes());
	}

	// Vertexをセットする
	auto* mapped_vertices = s_obj_vertex_resource->Map(0, &CD3DX12_RANGE(0u, 0u));

	s_obj_vertices_offset = 0u;
	for (const auto& obj : s_objects)
	{
		s_obj_vertices_offset += SCAST<UINT>(obj->GetVertex(&mapped_vertices[s_obj_vertices_offset]));
	}

	s_obj_vertex_resource->Unmap(0, &CD3DX12_RANGE(0u, 0u));

	return hr;
}

static bool RadioButtonHelper(std::string name, int* flg)
{
	ImGui::Text(name.c_str());
	int flg_log = *flg;
	ImGui::RadioButton(("ORIGINAL##" + std::to_string(RCAST<INT_PTR>(flg))).c_str(), flg, 0);
	ImGui::SameLine(); ImGui::RadioButton(("WIHITE##" + std::to_string(RCAST<INT_PTR>(flg))).c_str(), flg, 1);
	ImGui::SameLine(); ImGui::RadioButton(("BLACK##" + std::to_string(RCAST<INT_PTR>(flg))).c_str(), flg, 2);
	return flg_log != *flg;
}
void DebugManager::ChangeTexture(Texture* texture, int flg)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MipLevels = 1;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	ComPtr<ID3D12Device> device;
	texture->tex->Data()->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
	//RCHECK_HR(hr, "DebugManager::UpdateStaticObjects で GetDevice に失敗");

	switch (flg)
	{
		case 0:
		{
			srv_desc.Format = texture->tex->Data()->GetDesc().Format;
			device->CreateShaderResourceView(texture->tex->Data().Get(), &srv_desc, texture->handle.Cpu());
			break;
		}
		case 1:
		{
			srv_desc.Format = white_texture->Data()->GetDesc().Format;
			device->CreateShaderResourceView(white_texture->Data().Get(), &srv_desc, texture->handle.Cpu());
			break;
		}
		case 2:
		{
			srv_desc.Format = black_texture->Data()->GetDesc().Format;
			device->CreateShaderResourceView(black_texture->Data().Get(), &srv_desc, texture->handle.Cpu());
			break;
		}
	}
}

HRESULT DebugManager::Update(const TransformConstants& tc, const ShadingConstants& sc, bool use_gui)
{
	HRESULT hr = S_OK;
	if (s_obj_changed)
	{
		s_obj_changed = false;
		hr = UpdateStaticObjects();
	}

	{
		auto* mapped_buffer = s_obj_tc_resource->Map(0, &CD3DX12_RANGE(0u, 0u));
		*mapped_buffer = tc;
		s_obj_tc_resource->Unmap(0, &CD3DX12_RANGE(0u, 0u));
	}
	{
		auto* mapped_buffer = s_obj_sc_resource->Map(0, &CD3DX12_RANGE(0u, 0u));
		*mapped_buffer = sc;
		s_obj_sc_resource->Unmap(0, &CD3DX12_RANGE(0u, 0u));
	}

	if (use_gui)
	{
		if (ImGui::Begin("Debug"))
		{
			ImGui::Checkbox("Render", &render_flg);
			ImGui::Spacing();

			static int albedo = 0;
			if (RadioButtonHelper("albedo", &albedo))
				ChangeTexture(&s_obj_albedo, albedo);
			static int normal = 0;
			if (RadioButtonHelper("normal", &normal))
				ChangeTexture(&s_obj_normal, normal);
			static int metalness = 0;
			if (RadioButtonHelper("metalness", &metalness))
				ChangeTexture(&s_obj_metalness, metalness);
			static int roughness = 0;
			if (RadioButtonHelper("roughness", &roughness))
				ChangeTexture(&s_obj_roughness, roughness);
			static int specular = 0;
			if (RadioButtonHelper("specular", &specular))
				ChangeTexture(&s_obj_albedo, specular);
			static int irradiance = 0;
			if (RadioButtonHelper("irradiance", &irradiance))
				ChangeTexture(&s_obj_irradiance, irradiance);
			static int specular_brdf = 0;
			if (RadioButtonHelper("specular_brdf", &specular_brdf))
				ChangeTexture(&s_obj_specular_brdf, specular_brdf);
		}
		ImGui::End();
	}
	return hr;
}

void DebugManager::Render(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list, UINT msaa_count)
{
	if (render_flg)
	{
		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// スタティックオブジェクトの描画

		if (s_obj_renderers.size() > SCAST<UINT>(msaa_count))
		{
			if (s_obj_wire)
				s_obj_wire_renderers[msaa_count]->SetState(cmd_list);
			else
				s_obj_renderers[msaa_count]->SetState(cmd_list);

			cmd_list->SetDescriptorHeaps(1, s_obj_cbv_descmgr->Heap().GetAddressOf());
			cmd_list->SetGraphicsRootDescriptorTable(0, s_obj_tc_handle.Gpu());
			//cmd_list->SetGraphicsRootDescriptorTable(1, s_obj_sc_handle.Gpu());

			cmd_list->SetDescriptorHeaps(1, s_obj_srv_descmgr->Heap().GetAddressOf());
			cmd_list->SetGraphicsRootDescriptorTable(1, s_obj_albedo.handle.Gpu());

			cmd_list->IASetVertexBuffers(0, 1, &s_obj_vertex_view);
			cmd_list->DrawInstanced(s_obj_vertices_offset, 1, 0, 0);
		}
	}
}
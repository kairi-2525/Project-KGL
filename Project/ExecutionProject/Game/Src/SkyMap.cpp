#include "../Hrd/SkyMap.hpp"
#include <DirectXTex/d3dx12.h>
#include <Dx12/3D/Renderer.hpp>

#include <imgui.h>

SkyManager::SkyManager(KGL::ComPtrC<ID3D12Device> device,
	const std::shared_ptr<KGL::DXC>& dxc,
	std::shared_ptr<KGL::DescriptorManager> imgui_desc_mgr,
	DXGI_SAMPLE_DESC max_sample_desc, std::string folder,
	const std::vector<std::pair<std::string, std::string>>& textures,
	std::string extension)
{
	auto LoadSkyTex = [&](std::shared_ptr<Tex> data, std::string name0, std::string name1)
	{
		data->tex[CUBE::FRONT] = std::make_shared<KGL::Texture>(device, folder + name0 + "/" + name1 + "Cam_0_Front+Z" + extension);
		data->tex[CUBE::BACK] = std::make_shared<KGL::Texture>(device, folder + name0 + "/" + name1 + "Cam_1_Back-Z" + extension);
		data->tex[CUBE::RIGHT] = std::make_shared<KGL::Texture>(device, folder + name0 + "/" + name1 + "Cam_2_Left+X" + extension);
		data->tex[CUBE::LEFT] = std::make_shared<KGL::Texture>(device, folder + name0 + "/" + name1 + "Cam_3_Right-X" + extension);
		data->tex[CUBE::TOP] = std::make_shared<KGL::Texture>(device, folder + name0 + "/" + name1 + "Cam_4_Up+Y" + extension);
		data->tex[CUBE::BOTTOM] = std::make_shared<KGL::Texture>(device, folder + name0 + "/" + name1 + "Cam_5_Down-Y" + extension);
	};

	const size_t tex_num = TEXTURES.size();
	for (size_t i = 0u; i < tex_num; i++)
	{
		auto& data = tex_data[TEXTURES[i].first];
		data = std::make_shared<Tex>();
		LoadSkyTex(data, TEXTURES[i].first, TEXTURES[i].second);
	}

	std::vector<Vertex> sky_vertices(4 * CUBE::NUM);
	enum { TL, TR, BL, BR };
	const float one_texcel_size = 1.f / tex_data.cbegin()->second->tex[0]->Data()->GetDesc().Width;
	std::vector<DirectX::XMFLOAT2> uv(4);
	uv[TL] = { 0.f - one_texcel_size, 0.f - one_texcel_size };
	uv[TR] = { 1.f + one_texcel_size, 0.f - one_texcel_size };
	uv[BL] = { 0.f - one_texcel_size, 1.f + one_texcel_size };
	uv[BR] = { 1.f + one_texcel_size, 1.f + one_texcel_size };

	sky_vertices[CUBE::FRONT * 4 + TL] = { { -0.5f, +0.5f, +0.5f - one_texcel_size }, uv[TL] };
	sky_vertices[CUBE::FRONT * 4 + TR] = { { +0.5f, +0.5f, +0.5f - one_texcel_size }, uv[TR] };
	sky_vertices[CUBE::FRONT * 4 + BL] = { { -0.5f, -0.5f, +0.5f - one_texcel_size }, uv[BL] };
	sky_vertices[CUBE::FRONT * 4 + BR] = { { +0.5f, -0.5f, +0.5f - one_texcel_size }, uv[BR] };

	sky_vertices[CUBE::BACK * 4 + TL] = { { +0.5f, +0.5f, -0.5f + one_texcel_size }, uv[TL] };
	sky_vertices[CUBE::BACK * 4 + TR] = { { -0.5f, +0.5f, -0.5f + one_texcel_size }, uv[TR] };
	sky_vertices[CUBE::BACK * 4 + BL] = { { +0.5f, -0.5f, -0.5f + one_texcel_size }, uv[BL] };
	sky_vertices[CUBE::BACK * 4 + BR] = { { -0.5f, -0.5f, -0.5f + one_texcel_size }, uv[BR] };

	sky_vertices[CUBE::RIGHT * 4 + TL] = { { +0.5f - one_texcel_size, +0.5f, +0.5f }, uv[TL] };
	sky_vertices[CUBE::RIGHT * 4 + TR] = { { +0.5f - one_texcel_size, +0.5f, -0.5f }, uv[TR] };
	sky_vertices[CUBE::RIGHT * 4 + BL] = { { +0.5f - one_texcel_size, -0.5f, +0.5f }, uv[BL] };
	sky_vertices[CUBE::RIGHT * 4 + BR] = { { +0.5f - one_texcel_size, -0.5f, -0.5f }, uv[BR] };

	sky_vertices[CUBE::LEFT * 4 + TL] = { { -0.5f + one_texcel_size, +0.5f, -0.5f }, uv[TL] };
	sky_vertices[CUBE::LEFT * 4 + TR] = { { -0.5f + one_texcel_size, +0.5f, +0.5f }, uv[TR] };
	sky_vertices[CUBE::LEFT * 4 + BL] = { { -0.5f + one_texcel_size, -0.5f, -0.5f }, uv[BL] };
	sky_vertices[CUBE::LEFT * 4 + BR] = { { -0.5f + one_texcel_size, -0.5f, +0.5f }, uv[BR] };

	sky_vertices[CUBE::TOP * 4 + TL] = { { -0.5f, +0.5f - one_texcel_size, -0.5f }, uv[TL] };
	sky_vertices[CUBE::TOP * 4 + TR] = { { +0.5f, +0.5f - one_texcel_size, -0.5f }, uv[TR] };
	sky_vertices[CUBE::TOP * 4 + BL] = { { -0.5f, +0.5f - one_texcel_size, +0.5f }, uv[BL] };
	sky_vertices[CUBE::TOP * 4 + BR] = { { +0.5f, +0.5f - one_texcel_size, +0.5f }, uv[BR] };

	sky_vertices[CUBE::BOTTOM * 4 + TL] = { { -0.5f, -0.5f + one_texcel_size, +0.5f }, uv[TL] };
	sky_vertices[CUBE::BOTTOM * 4 + TR] = { { +0.5f, -0.5f + one_texcel_size, +0.5f }, uv[TR] };
	sky_vertices[CUBE::BOTTOM * 4 + BL] = { { -0.5f, -0.5f + one_texcel_size, -0.5f }, uv[BL] };
	sky_vertices[CUBE::BOTTOM * 4 + BR] = { { +0.5f, -0.5f + one_texcel_size, -0.5f }, uv[BR] };

	vbr = std::make_shared<KGL::Resource<Vertex>>(device, sky_vertices.size());

	auto* mapped_vertices = vbr->Map(0, &CD3DX12_RANGE(0, 0));
	std::copy(sky_vertices.cbegin(), sky_vertices.cend(), mapped_vertices);
	vbr->Unmap(0, &CD3DX12_RANGE(0, 0));

	auto buffer_location = vbr->Data()->GetGPUVirtualAddress();
	for (auto& vbv : vbv)
	{
		vbv.BufferLocation = buffer_location;
		vbv.SizeInBytes = sizeof(sky_vertices[0]) * 4;
		vbv.StrideInBytes = sizeof(sky_vertices[0]);
		buffer_location += vbv.SizeInBytes;
	}

	auto renderer_desc = KGL::_3D::Renderer::DEFAULT_DESC;
	renderer_desc.input_layouts.pop_back();
	renderer_desc.input_layouts.pop_back();
	renderer_desc.input_layouts.push_back(KGL::_3D::Renderer::INPUT_LAYOUTS[2]);
	renderer_desc.ps_desc.hlsl = "./HLSL/3D/UnNormal_ps.hlsl";
	renderer_desc.vs_desc.hlsl = "./HLSL/3D/UnNormal_vs.hlsl";
	renderer_desc.root_params.clear();
	D3D12_ROOT_PARAMETER param0 = { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
		{ { KGL::SCAST<UINT>(KGL::_3D::Renderer::DESCRIPTOR_RANGES0.size()), KGL::_3D::Renderer::DESCRIPTOR_RANGES0.data() } },
		D3D12_SHADER_VISIBILITY_VERTEX
	};
	D3D12_ROOT_PARAMETER param1 = { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
		{ { KGL::SCAST<UINT>(KGL::_3D::Renderer::DESCRIPTOR_RANGES2.size()), KGL::_3D::Renderer::DESCRIPTOR_RANGES2.data() } },
		D3D12_SHADER_VISIBILITY_PIXEL
	};
	renderer_desc.root_params.push_back(param0);
	renderer_desc.root_params.push_back(param1);

	renderer_desc.blend_types[0] = KGL::BLEND::TYPE::ALPHA;
	renderer_desc.rastarizer_desc.CullMode = D3D12_CULL_MODE_BACK;

	auto& sampler = renderer_desc.static_samplers[0];
	sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	renderers.push_back(std::make_shared<KGL::BaseRenderer>(device, dxc, renderer_desc));
	renderer_desc.rastarizer_desc.MultisampleEnable = TRUE;
	// MSAA—p
	for (; renderer_desc.other_desc.sample_desc.Count < max_sample_desc.Count;)
	{
		renderer_desc.other_desc.sample_desc.Count *= 2;
		renderers.push_back(std::make_shared<KGL::BaseRenderer>(device, dxc, renderer_desc));
	}

	desc_mgr = std::make_shared<KGL::DescriptorManager>(device, CUBE::NUM * tex_data.size() + 1);
	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MipLevels = 1;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	for (auto& data : tex_data)
	{
		for (int i = 0; i < CUBE::NUM; i++)
		{
			data.second->handle[i] = desc_mgr->Alloc();
			data.second->imgui_handle[i] = imgui_desc_mgr->Alloc();
			srv_desc.Format = data.second->tex[i]->Data()->GetDesc().Format;
			device->CreateShaderResourceView(
				data.second->tex[i]->Data().Get(),
				&srv_desc,
				data.second->handle[i].Cpu()
			);
			device->CreateShaderResourceView(
				data.second->tex[i]->Data().Get(),
				&srv_desc,
				data.second->imgui_handle[i].Cpu()
			);
		}
	}

	buffer = std::make_shared<KGL::Resource<DirectX::XMFLOAT4X4>>(device, sky_vertices.size());
	buffer_handle = desc_mgr->Alloc();
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
	cbv_desc.BufferLocation = buffer->Data()->GetGPUVirtualAddress();
	cbv_desc.SizeInBytes = SCAST<UINT>(buffer->SizeInBytes());
	device->CreateConstantBufferView(&cbv_desc, buffer_handle.Cpu());
}

void SkyManager::Init(DirectX::CXMMATRIX viewproj)
{
	using namespace DirectX;
	XMVECTOR box = XMVectorSet(1000.f * cosf(XMConvertToRadians(-45.f)), 1000.f * cosf(XMConvertToRadians(-45.f)), 1000.f * cosf(XMConvertToRadians(-45.f)), 0.f);
	scale = XMVector3Length(box).m128_f32[0] * 0.9f;
	select = tex_data.cbegin()->second;

	XMMATRIX W, S, R, T;
	S = XMMatrixScaling(scale, scale, scale);
	R = XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f);
	T = XMMatrixTranslation(0.f, 0.f, 0.f);
	W = S * R * T;
	XMMATRIX WVP = W * viewproj;
	SetWVP(WVP);

	render_flg = true;
}

void SkyManager::UpdateGui()
{
	const auto imgui_window_size = ImGui::GetWindowSize();
	ImGui::Checkbox("Render", &render_flg);
	ImGui::BeginChild("scrolling", ImVec2(imgui_window_size.x * 0.9f, std::max<float>(imgui_window_size.y - 100, 0)), ImGuiWindowFlags_NoTitleBar);
	for (auto& it : tex_data)
	{
		ImGui::Image((ImTextureID)it.second->imgui_handle[CUBE::FRONT].Gpu().ptr, ImVec2(90, 90));
		ImGui::SameLine();
		if (it.second == select)
		{
			ImGui::TextColored({ 0.8f, 0.8f, 0.8f, 1.f }, it.first.c_str());
		}
		else if (ImGui::Button(it.first.c_str()))
		{
			select = it.second;
		}
	}
	ImGui::EndChild();
	ImGui::SliderFloat("Scale", &scale, 1.f, 1000.f);
}
void SkyManager::Update(const DirectX::XMFLOAT3& pos, DirectX::CXMMATRIX viewproj)
{
	{
		using namespace DirectX;
		XMMATRIX W, S, R, T;
		S = XMMatrixScaling(scale, scale, scale);
		R = XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f);
		T = XMMatrixTranslation(pos.x, pos.y, pos.z);
		W = S * R * T;
		XMMATRIX WVP = W * viewproj;
		SetWVP(WVP);
	}
}

void SkyManager::Change(bool next)
{
	auto itr = tex_data.begin();
	while (itr != tex_data.end())
	{
		if (itr->second == select)
			break;
		itr++;
	}
	if (itr != tex_data.end())
	{
		if (next)
		{
			itr++;
			if (itr == tex_data.end())
				select = tex_data.begin()->second;
			else
				select = itr->second;
		}
		else
		{
			if (itr == tex_data.begin())
			{
				auto back = tex_data.end();
				back--;
				select = back->second;
			}
			else
			{
				itr--;
				select = itr->second;
			}
		}
	}
}

void SkyManager::SetWVP(DirectX::CXMMATRIX wvp)
{
	auto* p_mapped_wvp = buffer->Map(0, &CD3DX12_RANGE(0, 0));
	DirectX::XMStoreFloat4x4(p_mapped_wvp, wvp);
	buffer->Unmap(0, &CD3DX12_RANGE(0, 0));
}

void SkyManager::Render(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list, UINT msaa_scale)
{
	if (render_flg)
	{
		RCHECK(SCAST<size_t>(msaa_scale) >= renderers.size(), "msaa_scale ‚ª‘å‚«‚·‚¬‚Ü‚·");
		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		renderers[msaa_scale]->SetState(cmd_list);
		cmd_list->SetDescriptorHeaps(1, desc_mgr->Heap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, buffer_handle.Gpu());
		for (int i = 0; i < CUBE::NUM; i++)
		{
			cmd_list->IASetVertexBuffers(0, 1, &vbv[i]);
			cmd_list->SetGraphicsRootDescriptorTable(1, select->handle[i].Gpu());
			cmd_list->DrawInstanced(4, 1, 0, 0);
		}
	}
}

void SkyManager::Uninit(std::shared_ptr<KGL::DescriptorManager> imgui_desc_mgr)
{
	for (auto& it : tex_data)
	{
		for (int i = 0; i < CUBE::NUM; i++)
		{
			if (it.second)
			{
				imgui_desc_mgr->Free(it.second->imgui_handle[i]);
			}
		}
	}
}
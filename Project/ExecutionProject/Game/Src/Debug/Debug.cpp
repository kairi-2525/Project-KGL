#include "../../Hrd/Debug.hpp"
#include <DirectXTex/d3dx12.h>

DebugManager::DebugManager(ComPtrC<ID3D12Device> device, std::shared_ptr<KGL::BASE::DXC> dxc)
{
	s_obj_wire = false;
	s_obj_changed = false;

	s_obj_descmgr = std::make_shared<KGL::DescriptorManager>(device, 1u);
	s_obj_handle = s_obj_descmgr->Alloc();
	s_obj_buffer_resource = std::make_shared<KGL::Resource<CBuffer>>(device, 1u);
	s_obj_vertex_resource = std::make_shared<KGL::Resource<Vertex>>(device, 256u);
	s_objects.reserve(s_obj_vertex_resource->Size());

	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
		cbv_desc.BufferLocation = s_obj_buffer_resource->Data()->GetGPUVirtualAddress();
		cbv_desc.SizeInBytes = s_obj_buffer_resource->SizeInBytes();
		device->CreateConstantBufferView(&cbv_desc, s_obj_handle.Cpu());
	}

	s_obj_vertex_view.BufferLocation = s_obj_vertex_resource->Data()->GetGPUVirtualAddress();
	s_obj_vertex_view.SizeInBytes = s_obj_vertex_resource->SizeInBytes();
	s_obj_vertex_view.StrideInBytes = sizeof(Vertex);

	{
		auto renderer_desc = KGL::_3D::Renderer::DEFAULT_DESC;
		renderer_desc.input_layouts.clear();
		renderer_desc.input_layouts.push_back({
			"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });

		renderer_desc.root_params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		renderer_desc.root_params.pop_back();
		renderer_desc.root_params.pop_back();

		renderer_desc.vs_desc.hlsl = "./HLSL/3D/StaticObject_vs.hlsl";
		renderer_desc.ps_desc.hlsl = "./HLSL/3D/StaticObject_ps.hlsl";
		s_obj_renderer = std::make_shared<KGL::_3D::Renderer>(device, dxc, renderer_desc);

		renderer_desc.rastarizer_desc.FillMode = D3D12_FILL_MODE_WIREFRAME;
		s_obj_wire_renderer = std::make_shared<KGL::_3D::Renderer>(device, dxc, renderer_desc);
	}
}

void DebugManager::AddStaticObjects(const std::vector<Object>& objects)
{
	if (!objects.empty())
	{
		s_obj_changed = true;
		s_objects.reserve(s_objects.size() + objects.size());
		std::copy(objects.begin(), objects.end(), std::back_inserter(s_objects));
	}
}

HRESULT DebugManager::UpdateStaticObjects()
{
	HRESULT hr = S_OK;

	// リソースのサイズが足りない場合
	if (s_obj_vertex_resource->Size() < s_objects.size())
	{
		ComPtr<ID3D12Device> device;
		hr = s_obj_vertex_resource->Data()->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
		RCHECK_HR(hr, "DebugManager::UpdateStaticObjects で GetDevice に失敗");
		s_obj_vertex_resource = std::make_shared<KGL::Resource<Vertex>>(device, s_objects.size());

		s_obj_vertex_view.BufferLocation = s_obj_vertex_resource->Data()->GetGPUVirtualAddress();
		s_obj_vertex_view.SizeInBytes = s_obj_vertex_resource->SizeInBytes();
	}

	// Vertexをセットする
	auto* mapped_vertices = s_obj_vertex_resource->Map(0, &CD3DX12_RANGE(0u, 0u));

	size_t vertices_offset = 0u;
	for (const auto& obj : s_objects)
	{
		vertices_offset += obj.GetVertex(&mapped_vertices[vertices_offset]);
	}

	s_obj_vertex_resource->Unmap(0, &CD3DX12_RANGE(0u, 0u));

	return hr;
}

HRESULT DebugManager::Update(DirectX::CXMMATRIX view_proj)
{
	HRESULT hr = S_OK;
	if (s_obj_changed)
	{
		s_obj_changed = false;
		hr = UpdateStaticObjects();
	}

	auto* mapped_buffer = s_obj_buffer_resource->Map(0, &CD3DX12_RANGE(0u, 0u));
	DirectX::XMStoreFloat4x4(&mapped_buffer->view_projection, view_proj);
	s_obj_buffer_resource->Unmap(0, &CD3DX12_RANGE(0u, 0u));
	return hr;
}

void DebugManager::Render(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list)
{
	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	const UINT s_obj_count = SCAST<UINT>(s_objects.size());
	// スタティックオブジェクトの描画
	if (s_obj_count > 0u)
	{
		if (s_obj_wire)
			s_obj_wire_renderer->SetState(cmd_list);
		else
			s_obj_renderer->SetState(cmd_list);

		cmd_list->SetDescriptorHeaps(1, s_obj_handle.Heap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, s_obj_handle.Gpu());

		cmd_list->IASetVertexBuffers(0, 1, &s_obj_vertex_view);
		cmd_list->DrawInstanced(s_obj_count, 1, 0, 0);
	}
}
#include <Dx12/3D/StaticModelActor.hpp>

using namespace KGL;

StaticModelActor::StaticModelActor(
	ComPtrC<ID3D12Device> device,
	std::shared_ptr<const StaticModel> model
) noexcept :
	m_model(model)
{
	const auto& mtls = m_model->GetMaterials();
	const auto mtl_size = mtls.size();

	m_descriptor = std::make_shared<KGL::DescriptorManager>(device, 1u + mtl_size);

	// モデルバッファを作成
	m_model_buffer = std::make_shared<KGL::Resource<ModelBuffer>>(device, 1u);
	m_model_buffer_handle = std::make_shared<KGL::DescriptorHandle>(m_descriptor->Alloc());
	m_model_buffer->CreateCBV(m_model_buffer_handle);

	// マテリアルバッファを作成
	m_mtl_buffers.reserve(mtl_size);
	for (const auto& it : mtls)
	{
		auto& mtl = m_mtl_buffers[it.first];
		mtl.resource = std::make_shared<KGL::Resource<MaterialBuffer>>(device, 1u);
		mtl.handle = std::make_shared<KGL::DescriptorHandle>(m_descriptor->Alloc());
		mtl.resource->CreateCBV(mtl.handle);
	}
}

StaticModelActor& StaticModelActor::operator=(const StaticModelActor& m) noexcept
{
	{	// 親クラスのパラメーターのみコピー
		Actor* p = this;
		const Actor* mp = &m;
		*p = *mp;
	}

	m_model = m.m_model;

	ComPtr<ID3D12Device> device;
	m.m_model_buffer->Data()->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));

	const auto mtl_size = m.m_mtl_buffers.size();
	m_descriptor = std::make_shared<KGL::DescriptorManager>(device, 1u + mtl_size);

	// モデルバッファを作成
	m_model_buffer = std::make_shared<KGL::Resource<ModelBuffer>>(*m.m_model_buffer);
	m_model_buffer_handle = std::make_shared<KGL::DescriptorHandle>(m_descriptor->Alloc());
	m_model_buffer->CreateCBV(m_model_buffer_handle);

	// マテリアルバッファを作成
	m_mtl_buffers.reserve(mtl_size);
	for (auto& it : m.m_mtl_buffers)
	{
		auto& mtl = m_mtl_buffers[it.first];
		mtl.resource = std::make_shared<KGL::Resource<MaterialBuffer>>(*it.second.resource);
		mtl.handle = std::make_shared<KGL::DescriptorHandle>(m_descriptor->Alloc());
		mtl.resource->CreateCBV(mtl.handle);
	}

	return *this;
}

void StaticModelActor::UpdateBuffer(DirectX::CXMMATRIX view_proj)
{
	{
		auto* model_buffer = m_model_buffer->Map();
		const auto W = GetWorldMatrix();
		DirectX::XMStoreFloat4x4(&model_buffer->world, W);
		DirectX::XMStoreFloat4x4(&model_buffer->wvp, W * view_proj);
		m_model_buffer->Unmap();
	}
}

void StaticModelActor::Render(ComPtrC<ID3D12GraphicsCommandList> cmd_list) const noexcept
{
	cmd_list->SetDescriptorHeaps(1, m_descriptor->Heap().GetAddressOf());

	// モデルバッファをセット
	cmd_list->SetGraphicsRootDescriptorTable(1, m_model_buffer_handle->Gpu());

	const auto& mtls = m_model->GetMaterials();
	for (const auto& it : mtls)
	{
		const auto& mtl = it.second;
		const auto& mtl_buff = m_mtl_buffers.at(it.first);

		// マテリアルバッファをセット
		cmd_list->SetGraphicsRootDescriptorTable(2, mtl_buff.handle->Gpu());

		cmd_list->IASetVertexBuffers(0, 1, &mtl.vertex_buffer_view);
		cmd_list->DrawInstanced(SCAST<UINT>(mtl.rs_vertices->Size()), 1, 0, 0);
	}
}
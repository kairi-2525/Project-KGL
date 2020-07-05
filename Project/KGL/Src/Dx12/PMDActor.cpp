#include <Dx12/PMDActor.hpp>
#include <Helper/Cast.hpp>
#include <Helper/ThrowAssert.hpp>
#include <DirectXTex/d3dx12.h>

using namespace KGL;

void PMD_Actor::RecursiveMatrixMultiply(
	const PMD::BoneNode* node,
	const DirectX::XMMATRIX& mat
) noexcept
{
	m_map_buffers->bones[node->bone_idx] *= mat;
	for (auto& cnode : node->children)
	{
		RecursiveMatrixMultiply(cnode, m_map_buffers->bones[node->bone_idx]);
	}
}

PMD_Actor::PMD_Actor(
	const ComPtr<ID3D12Device>& device,
	const PMD_Model& model
) noexcept
	: m_map_buffers(nullptr), m_bone_table(model.GetBoneTable())
{
	// 定数バッファの作成
	auto hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstantBuffers) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_const_buff.ReleaseAndGetAddressOf())
	);
	RCHECK(FAILED(hr), "CreateCommittedResourceに失敗");
	hr = m_const_buff->Map(0, nullptr, (void**)&m_map_buffers);
	RCHECK(FAILED(hr), "const_buff->Mapに失敗");

	{
		m_map_buffers->world = DirectX::XMMatrixIdentity();
		const auto& bones = model.GetBoneMatrices();
		std::copy(bones.cbegin(), bones.cend(), m_map_buffers->bones);
	}

	D3D12_DESCRIPTOR_HEAP_DESC desc_heap_desc = {};
	// シェーダーから見えるように
	desc_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc_heap_desc.NodeMask = 0;
	// CBV
	desc_heap_desc.NumDescriptors = 1;
	// シェーダーリソースビュー用
	desc_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	hr = device->CreateDescriptorHeap(&desc_heap_desc, IID_PPV_ARGS(m_desc_heap.ReleaseAndGetAddressOf()));
	RCHECK(FAILED(hr), "CreateDescriptorHeapに失敗");
	D3D12_CPU_DESCRIPTOR_HANDLE basic_heap_handle(m_desc_heap->GetCPUDescriptorHandleForHeapStart());

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
	cbv_desc.BufferLocation = m_const_buff->GetGPUVirtualAddress();
	cbv_desc.SizeInBytes = KGL::SCAST<UINT>(m_const_buff->GetDesc().Width);
	device->CreateConstantBufferView(&cbv_desc, basic_heap_handle);
}

void PMD_Actor::SetAnimation(const VMD::Desc& desc) noexcept
{
	using namespace DirectX;

	const auto& motion_data = desc.motion_data;
	for (const auto& bone_motion : motion_data)
	{
		const auto& node = m_bone_table->at(bone_motion.first);
		const auto& pos = node.start_pos;
		m_map_buffers->bones[node.bone_idx] =
			XMMatrixTranslation(-pos.x, -pos.y, -pos.z)
			* XMMatrixRotationQuaternion(bone_motion.second[0].quaternion)
			* XMMatrixTranslation(pos.x, pos.y, pos.z);
	}
	const auto& node = m_bone_table->at("センター");
	RecursiveMatrixMultiply(
		&node,
		XMMatrixIdentity()
	);
}

void PMD_Actor::MotionUpdate(float elapsed_time) noexcept
{
	const UINT frame_no = SCAST<UINT>(30 * (elapsed_time / 1000.f));
}

void PMD_Actor::UpdateWVP()
{
	m_map_buffers->wvp = m_map_buffers->world * m_map_buffers->view * m_map_buffers->proj;
}

void PMD_Actor::Render(
	const ComPtr<ID3D12GraphicsCommandList>& cmd_list
) const noexcept
{
	cmd_list->SetDescriptorHeaps(1, m_desc_heap.GetAddressOf());
	auto heap_handle = m_desc_heap->GetGPUDescriptorHandleForHeapStart();
	cmd_list->SetGraphicsRootDescriptorTable(
		0,	// ルートパラメーターインデックス
		heap_handle
	);
}
#include <Dx12/PMDActor.hpp>
#include <Helper/Cast.hpp>
#include <Helper/ThrowAssert.hpp>
#include <Helper/Matrix.hpp>
#include <DirectXTex/d3dx12.h>

using namespace KGL;

// ベジェ曲線を計算する
float PMD_Actor::GetYFromXOnBezier(
	float x,
	const DirectX::XMFLOAT2& a, const DirectX::XMFLOAT2& b,
	UINT8 n
) noexcept
{
	if (a.x == a.y && b.x == b.y)
		return x;	// 計算不要

	float t = x;
	const float k0 = 1 + 3 * a.x - 3 * b.x;		// t^3の係数
	const float k1 = 3 * b.x - 6 * a.x;			// t^2の係数
	const float k2 = 3 * a.x;					// tの係数

	// 誤差の範囲内かどうかに使用する定数
	constexpr float EPSILON = 0.0005f;
	
	// tを近似で求める
	for (int i = 0; i < n; ++i)
	{
		// f(t)を求める
		auto ft =
			k0 * t * t * t
			+ k1 * t * t
			+ k2 * t - x;
		// もし結果が誤差の範囲なら打ち切る
		if (ft <= EPSILON && ft >= -EPSILON)
			break;
		t -= ft / 2; // 刻む
	}

	// yを計算する
	auto r = 1 - t;
	return 
		t * t * t
		+ 3 * t * t * r * b.y
		+ 3 * t * r * r * a.y;
}

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
	: m_map_buffers(nullptr), m_model_desc(model.GetDesc()), m_anim_counter(0.f)
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

void PMD_Actor::SetAnimation(const std::shared_ptr<const VMD::Desc>& desc) noexcept
{
	using namespace DirectX;
	m_anim_desc = desc;

	m_anim_counter = 0.f;
	const auto& motion_data = desc->motion_data;
	const auto& bone_table = m_model_desc->bone_node_table;
	for (const auto& bone_motion : motion_data)
	{
		auto node = bone_table.find(bone_motion.first);
		if (node == bone_table.cend()) continue;
		const auto& pos = node->second.start_pos;
		m_map_buffers->bones[node->second.bone_idx] =
			XMMatrixTranslation(-pos.x, -pos.y, -pos.z)
			* XMMatrixRotationQuaternion(bone_motion.second[0].quaternion)
			* XMMatrixTranslation(pos.x, pos.y, pos.z);
	}
	const auto& node = bone_table.at("センター");
	RecursiveMatrixMultiply(
		&node,
		XMMatrixIdentity()
	);
}

void PMD_Actor::ClearAnimation() noexcept
{
	std::fill(
		std::begin(m_map_buffers->bones),
		std::end(m_map_buffers->bones),
		DirectX::XMMatrixIdentity()
	);
}

void PMD_Actor::MotionUpdate(float elapsed_time, bool loop, bool bezier) noexcept
{
	using namespace DirectX;
	if (m_anim_desc)
	{
		m_anim_counter += elapsed_time;
		UINT frame_no = SCAST<UINT>(30 * m_anim_counter);
		if (frame_no > m_anim_desc->max_frame)
		{
			if (loop)
			{
				while (frame_no > m_anim_desc->max_frame)
				{
					m_anim_counter -= SCAST<float>(m_anim_desc->max_frame) / 30;
					frame_no = SCAST<UINT>(30 * m_anim_counter);
				}
			}
			else m_anim_counter = SCAST<float>(frame_no) / 30;
		}
		ClearAnimation();

		// モーションデータ更新
		const auto& motion_data = m_anim_desc->motion_data;
		const auto& bone_table = m_model_desc->bone_node_table;
		for (const auto& bone_motion : motion_data)
		{
			const auto& node = bone_table.at(bone_motion.first);
			const auto& motions = bone_motion.second;
			// フレームナンバーが小さい順に並んでいることが前提
			auto ritr = std::find_if(
				motions.crbegin(), motions.crend(),
				[frame_no](const KGL::VMD::Key_Frame& motion)
				{
					return motion.frame_no <= frame_no;
				}
			);
			if (ritr == motions.crend())
			{
				continue;
			}
			// reverseイテレータを普通のイテレーターに変換する際に一つずれるのでそれが次のイテレーターになる。
			auto itr = ritr.base();
			XMMATRIX rotation;
			if (itr != motions.cend())
			{
				auto t = SCAST<float>(frame_no - ritr->frame_no) / SCAST<float>(itr->frame_no - ritr->frame_no);
				if (bezier) t = GetYFromXOnBezier(t, itr->p1, itr->p2, 12);
				rotation =
					XMMatrixRotationQuaternion(
						XMQuaternionSlerp(ritr->quaternion, itr->quaternion, t)
					);
			}
			else
				rotation = XMMatrixRotationQuaternion(ritr->quaternion);

			const auto& pos = node.start_pos;

			m_map_buffers->bones[node.bone_idx] =
				XMMatrixTranslation(-pos.x, -pos.y, -pos.z)
				* rotation
				* XMMatrixTranslation(pos.x, pos.y, pos.z);
		}
		const auto& node = bone_table.at("センター");
		RecursiveMatrixMultiply(
			&node,
			XMMatrixIdentity()
		);
	}
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

// 場合分け
void PMD_Actor::IKSolve() noexcept
{
	for (auto& ik : m_model_desc->ik_data)
	{
		auto children_nodes_count = ik.node_idxes.size();
		switch (children_nodes_count)
		{
			case 0: assert(0); continue;
			case 1: SolveLookAt(ik); break;
			case 2: SolveCosineIK(ik); break;
			default: SolveCCDIK(ik); break;
		}
	}
}

// CCD-IKによりボーン方向を解決
void PMD_Actor::SolveCCDIK(const PMD::IK& ik) noexcept
{

}

// 余弦定理IKによりボーン方向を解決
void PMD_Actor::SolveCosineIK(const PMD::IK& ik) noexcept
{

}

// LookAt行列によりボーン方向を解決
void PMD_Actor::SolveLookAt(const PMD::IK& ik) noexcept
{
	using namespace DirectX;
	// この関数に来た時点でノードは１つしかない
	// チェーンに入っているノード番号はIKのルートノードの物なので、
	// このルートノードから末端に向かうベクトルを考える

	auto root_node = m_model_desc->bone_node_address_array[ik.node_idxes[0]];
	auto target_node = m_model_desc->bone_node_address_array[ik.bone_idx];

	auto rpos1 = XMLoadFloat3(&root_node->start_pos);
	auto tpos1 = XMLoadFloat3(&target_node->start_pos);

	auto rpos2 = XMVector3TransformCoord(rpos1, m_map_buffers->bones[ik.node_idxes[0]]);
	auto tpos2 = XMVector3TransformCoord(tpos1, m_map_buffers->bones[ik.bone_idx]);

	auto origin_vec = XMVector3Normalize(XMVectorSubtract(tpos1, rpos1));
	auto target_vec = XMVector3Normalize(XMVectorSubtract(tpos2, rpos2));

	m_map_buffers->bones[ik.node_idxes[0]] = 
		XMLookAtMatrix(
			origin_vec, target_vec,
			XMVectorSet(0.f, 1.f, 0.f, 0.f),
			XMVectorSet(1.f, 0.f, 0.f, 0.f)
		);
}
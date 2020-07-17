#include <Dx12/3D/PMDActor.hpp>
#include <Helper/Cast.hpp>
#include <Helper/ThrowAssert.hpp>
#include <Math/Matrix.hpp>
#include <DirectXTex/d3dx12.h>
#include <array>

using namespace KGL;

// ベジェ曲線を計算する
float PMD_Actor::GetYFromXOnBezier(
	float x,
	const DirectX::XMFLOAT2& a,
	const DirectX::XMFLOAT2& b,
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
	const DirectX::XMMATRIX& mat
) noexcept
{
	const auto& bone_table = m_model_desc->bone_node_table;
	const auto& node = bone_table.at("センター");
	m_map_buffers->bones[node.bone_idx] *= mat;
	for (auto& cnode : node.children)
	{
		RecursiveMatrixMultiply(cnode, m_map_buffers->bones[node.bone_idx]);
	}
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

void PMD_Actor::MotionSetup(float elapsed_time, bool loop, bool bezier) noexcept
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
			if (bone_table.count(bone_motion.first) == 0u) continue;
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
	}
}

void PMD_Actor::MotionMatrixUpdate(DirectX::CXMMATRIX mat) noexcept
{
	RecursiveMatrixMultiply(
		mat
	);
}

void PMD_Actor::IKUpdate()
{
	UINT frame_no = SCAST<UINT>(30 * m_anim_counter);
	IKSolve(frame_no);
}

void PMD_Actor::UpdateWVP(DirectX::CXMMATRIX view_proj)
{
	m_map_buffers->wvp = m_map_buffers->world * view_proj;
}

void PMD_Actor::Render(
	const ComPtr<ID3D12GraphicsCommandList>& cmd_list
) const noexcept
{
	cmd_list->SetDescriptorHeaps(1, m_desc_heap.GetAddressOf());
	auto heap_handle = m_desc_heap->GetGPUDescriptorHandleForHeapStart();
	cmd_list->SetGraphicsRootDescriptorTable(
		1,	// ルートパラメーターインデックス
		heap_handle
	);
}

// 場合分け
void PMD_Actor::IKSolve(UINT frame_no) noexcept
{
	// IKをスキップするか確認
	auto ik_itr =
		std::find_if(
			m_anim_desc->ik_enable_data.crbegin(),
			m_anim_desc->ik_enable_data.crend(),
			[frame_no](const VMD::IKEnable& ik_enable)
			{
				return ik_enable.frame_no <= frame_no;
			}
	);

	for (auto& ik : m_model_desc->ik_data)
	{
		// IKのON / OFF 確認
		if (ik_itr != m_anim_desc->ik_enable_data.crend())
		{
			auto ik_enable_it = 
				ik_itr->ik_enable_table.find(
					m_model_desc->bone_name_array[ik.bone_idx]
				);
			if (ik_enable_it != ik_itr->ik_enable_table.end())
			{
				if (!ik_enable_it->second)
				{	// もしオフなら打ち切る
					continue;
				}
			}
		}
		// 実行
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
	using namespace DirectX;

	// IK構成点を保存
	std::vector<XMVECTOR> bone_positions;
	const auto ik_limit = ik.limit * XM_PI;

	const auto& address_array = m_model_desc->bone_node_address_array;
	auto target_bone_node = address_array[ik.bone_idx];
	auto target_origin_pos = XMLoadFloat3(&target_bone_node->start_pos);

	auto parent_mat = m_map_buffers->bones[target_bone_node->ik_parent_bone];
	XMVECTOR det;
	auto inv_parent_mat = XMMatrixInverse(&det, parent_mat);
	auto target_next_pos = 
		XMVector3Transform(
			target_origin_pos,
			m_map_buffers->bones[ik.bone_idx] * inv_parent_mat
		);

	// 末端ノード
	auto end_pos = XMLoadFloat3(&address_array[ik.target_idx]->start_pos);

	// 中間ノード(ルートを含む)
	for (auto& cidx : ik.node_idxes)
	{
		bone_positions.push_back(XMLoadFloat3(&address_array[cidx]->start_pos));
	}
	const auto positions_size = bone_positions.size();
	std::vector<XMMATRIX> mats(positions_size);
	std::fill(mats.begin(), mats.end(), XMMatrixIdentity());

	// ikに設定されている試行回数だけ繰り返す
	for (int c = 0; c < ik.iterations; c++)
	{
		// ターゲットと末端がほぼ一致していたら抜ける
		if (XMVector3Length(XMVectorSubtract(end_pos, target_next_pos)).m128_f32[0] <= FLT_EPSILON)
		{
			break;
		}
		// それぞれのボーンをさかのぼりながら
		// 角度制限に引っかからないように曲げていく
		// bone_positionsは CCD-IKにおける各ノードの座標をVECTOR配列にしたもの
		for (int bidx = 0; bidx < positions_size; bidx++)
		{
			const auto& pos = bone_positions[bidx];
			// 対象ノードから末端ノードまでと
			// 対象ノードからターゲットまでのベクトルを作成
			auto vec_to_end = XMVectorSubtract(end_pos, pos);				// 末端へ
			auto vec_to_target = XMVectorSubtract(target_next_pos, pos);	// ターゲットへ
			// 両方正規化
			vec_to_end = XMVector3Normalize(vec_to_end);
			vec_to_target = XMVector3Normalize(vec_to_target);

			// ほぼ同じベクトルになってしまった場合は
			// 外積できないため次のボーンに引き渡す
			if (XMVector3Length(XMVectorSubtract(vec_to_end, vec_to_target)).m128_f32[0] < FLT_EPSILON)
			{
				continue;
			}

			// 外積計算及び角度計算
			auto cross = XMVector3Normalize(XMVector3Cross(vec_to_end, vec_to_target)); // 軸になる
			// 便利な関数だが中身はcos(内積値)なので 0~90°と0~-90°の区別がつかない
			float angle = XMVector3AngleBetweenVectors(vec_to_end, vec_to_target).m128_f32[0];
			// 回転限界を超えてしまったときは限界値に補正
			angle = (std::min)(angle, ik_limit);
			XMMATRIX rot = XMMatrixRotationAxis(cross, angle);
			// 原点中心ではなくpos中心に回転
			auto mat =
				XMMatrixTranslationFromVector(-pos)
				* rot
				* XMMatrixTranslationFromVector(pos);

			// 回転行列を保持しておく(乗算で回転重ね掛けを作っておく)
			mats[bidx] *= mat;

			// 対象となる点をすべて回転させる(現在の点からみて末端側を回転)
			// なお、自分を回転させる必要はない
			for (auto idx = bidx - 1; idx >= 0; idx--)
			{
				bone_positions[idx] = XMVector3Transform(bone_positions[idx], mat);
			}
			
			end_pos = XMVector3Transform(end_pos, mat);
			// もし正解に近くなっていたらループを抜ける
			if (XMVector3Length(XMVectorSubtract(end_pos, target_next_pos)).m128_f32[0] <= FLT_EPSILON)
			{
				break;
			}
		}
	}

	int idx = 0;
	for (auto& cidx : ik.node_idxes)
	{
		m_map_buffers->bones[cidx] = mats[idx];
		idx++;
	}
	auto root_node = address_array[ik.node_idxes.back()];
	RecursiveMatrixMultiply(root_node, parent_mat);
}

// 余弦定理IKによりボーン方向を解決
void PMD_Actor::SolveCosineIK(const PMD::IK& ik) noexcept
{
	using namespace DirectX;

	// IK構成点を保存
	std::vector<XMVECTOR> positions;
	// IKのそれぞれのボーンの座標を取得しておきます。
	std::array<float, 2> edge_lens;

	auto target_node = m_model_desc->bone_node_address_array[ik.bone_idx];
	auto target_pos = XMVector3Transform(
		XMLoadFloat3(&target_node->start_pos),
		m_map_buffers->bones[ik.bone_idx]
	);

	// IKチェーンが逆順なので、逆に並ぶようにしているa
	// 末端ボーン
	auto end_node = m_model_desc->bone_node_address_array[ik.target_idx];
	positions.emplace_back(XMLoadFloat3(&end_node->start_pos));

	// 中間及びルートボーン
	for (auto& chain_bone_idx : ik.node_idxes)
	{
		auto bone_node = m_model_desc->bone_node_address_array[chain_bone_idx];
		positions.emplace_back(XMLoadFloat3(&bone_node->start_pos));
	}

	//分かりづらいので逆にする
	std::reverse(positions.begin(), positions.end());

	// 元の長さを図っておく
	edge_lens[0] = XMVector3Length(XMVectorSubtract(positions[1], positions[0])).m128_f32[0];
	edge_lens[1] = XMVector3Length(XMVectorSubtract(positions[2], positions[1])).m128_f32[0];

	// ルートボーン座標変換(逆順になっているため使用するインデックスに注意)
	positions[0] = XMVector3Transform(
		positions[0],
		m_map_buffers->bones[ik.node_idxes[1]]
	);

	// 真ん中は自動計算されるので計算しない
	// 先端ボーン
	positions[2] = XMVector3Transform(
		positions[2], 
		m_map_buffers->bones[ik.bone_idx]
	);

	// ルートから先端へのベクトルを作っておく
	auto linear_vec = XMVectorSubtract(positions[2], positions[0]);
	float a = XMVector3Length(linear_vec).m128_f32[0];
	float b = edge_lens[0];
	float c = edge_lens[1];

	linear_vec = XMVector3Normalize(linear_vec);

	// ルートから真ん中への角度計算
	float theta1 = acosf((a * a + b * b - c * c) / (2 * a * b));
	// 真ん中からターゲットへの角度計算
	float theta2 = acosf((b * b + c * c - a * a) / (2 * b * c));

	// 「軸」を求める
	// もし真ん中が「ひざ」であった場合には強制的にX軸とする
	XMVECTOR axis;
	if (std::find(
			m_model_desc->knee_idxes.cbegin(),
			m_model_desc->knee_idxes.cend(),
			ik.node_idxes[0])
		== m_model_desc->knee_idxes.cend()) 
	{
		auto vm = XMVector3Normalize(
			XMVectorSubtract(positions[2], positions[0])
		);
		auto vt = XMVector3Normalize(
			XMVectorSubtract(target_pos, positions[0])
		);
		axis = XMVector3Cross(vt, vm);
	}
	else // ひざの場合
	{
		// 右
		axis = XMVectorSet(1.f, 0.f, 0.f, 0.f);

		/*auto vm = XMVector3Normalize(
			XMVectorSubtract(positions[2], positions[0])
		);
		auto vt = XMVector3Normalize(
			XMVectorSubtract(target_pos, positions[0])
		);

		axis = XMVector3Cross(vt, vm);

		float length_sq;
		XMStoreFloat(&length_sq, XMVector3LengthSq(axis));
		if (length_sq == 0.0f)
			axis = XMVectorSet(1.f, 0.f, 0.f, 0.f);*/
	}

	// IKチェーンはルートに向かってから数えられるため１がルートに近い
	auto mat1 = XMMatrixTranslationFromVector(-positions[0]);
	mat1 *= XMMatrixRotationAxis(axis, theta1);
	mat1 *= XMMatrixTranslationFromVector(positions[0]);

	auto mat2 = XMMatrixTranslationFromVector(-positions[1]);
	mat2 *= XMMatrixRotationAxis(axis, theta2 - XM_PI);
	mat2 *= XMMatrixTranslationFromVector(positions[1]);

	m_map_buffers->bones[ik.node_idxes[1]] *= mat1;
	m_map_buffers->bones[ik.node_idxes[0]] = mat2 * m_map_buffers->bones[ik.node_idxes[1]];
	m_map_buffers->bones[ik.target_idx] = m_map_buffers->bones[ik.node_idxes[0]];
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
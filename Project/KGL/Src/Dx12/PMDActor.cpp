#include <Dx12/PMDActor.hpp>
#include <Helper/Cast.hpp>
#include <Helper/ThrowAssert.hpp>
#include <Helper/Matrix.hpp>
#include <DirectXTex/d3dx12.h>
#include <array>

using namespace KGL;

// �x�W�F�Ȑ����v�Z����
float PMD_Actor::GetYFromXOnBezier(
	float x,
	const DirectX::XMFLOAT2& a, const DirectX::XMFLOAT2& b,
	UINT8 n
) noexcept
{
	if (a.x == a.y && b.x == b.y)
		return x;	// �v�Z�s�v

	float t = x;
	const float k0 = 1 + 3 * a.x - 3 * b.x;		// t^3�̌W��
	const float k1 = 3 * b.x - 6 * a.x;			// t^2�̌W��
	const float k2 = 3 * a.x;					// t�̌W��

	// �덷�͈͓̔����ǂ����Ɏg�p����萔
	constexpr float EPSILON = 0.0005f;
	
	// t���ߎ��ŋ��߂�
	for (int i = 0; i < n; ++i)
	{
		// f(t)�����߂�
		auto ft =
			k0 * t * t * t
			+ k1 * t * t
			+ k2 * t - x;
		// �������ʂ��덷�͈̔͂Ȃ�ł��؂�
		if (ft <= EPSILON && ft >= -EPSILON)
			break;
		t -= ft / 2; // ����
	}

	// y���v�Z����
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
	// �萔�o�b�t�@�̍쐬
	auto hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstantBuffers) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_const_buff.ReleaseAndGetAddressOf())
	);
	RCHECK(FAILED(hr), "CreateCommittedResource�Ɏ��s");
	hr = m_const_buff->Map(0, nullptr, (void**)&m_map_buffers);
	RCHECK(FAILED(hr), "const_buff->Map�Ɏ��s");

	{
		m_map_buffers->world = DirectX::XMMatrixIdentity();
		const auto& bones = model.GetBoneMatrices();
		std::copy(bones.cbegin(), bones.cend(), m_map_buffers->bones);
	}

	D3D12_DESCRIPTOR_HEAP_DESC desc_heap_desc = {};
	// �V�F�[�_�[���猩����悤��
	desc_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc_heap_desc.NodeMask = 0;
	// CBV
	desc_heap_desc.NumDescriptors = 1;
	// �V�F�[�_�[���\�[�X�r���[�p
	desc_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	hr = device->CreateDescriptorHeap(&desc_heap_desc, IID_PPV_ARGS(m_desc_heap.ReleaseAndGetAddressOf()));
	RCHECK(FAILED(hr), "CreateDescriptorHeap�Ɏ��s");
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
	const auto& node = bone_table.at("�Z���^�[");
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

		// ���[�V�����f�[�^�X�V
		const auto& motion_data = m_anim_desc->motion_data;
		const auto& bone_table = m_model_desc->bone_node_table;
		for (const auto& bone_motion : motion_data)
		{
			const auto& node = bone_table.at(bone_motion.first);
			const auto& motions = bone_motion.second;
			// �t���[���i���o�[�����������ɕ���ł��邱�Ƃ��O��
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
			// reverse�C�e���[�^�𕁒ʂ̃C�e���[�^�[�ɕϊ�����ۂɈ�����̂ł��ꂪ���̃C�e���[�^�[�ɂȂ�B
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
		const auto& node = bone_table.at("�Z���^�[");
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
		0,	// ���[�g�p�����[�^�[�C���f�b�N�X
		heap_handle
	);
}

// �ꍇ����
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

// CCD-IK�ɂ��{�[������������
void PMD_Actor::SolveCCDIK(const PMD::IK& ik) noexcept
{

}

// �]���藝IK�ɂ��{�[������������
void PMD_Actor::SolveCosineIK(const PMD::IK& ik) noexcept
{
	using namespace DirectX;

	// IK�\���_��ۑ�
	std::vector<XMVECTOR> positions;
	// IK�̂��ꂼ��̃{�[���̍��W���擾���Ă����܂��B
	std::array<float, 2> edge_lens;

	auto target_node = m_model_desc->bone_node_address_array[ik.bone_idx];
	auto target_pos = XMVector3Transform(
		XMLoadFloat3(&target_node->start_pos),
		m_map_buffers->bones[ik.bone_idx]
	);

	// IK�`�F�[�����t���Ȃ̂ŁA�t�ɕ��Ԃ悤�ɂ��Ă���a
	// ���[�{�[��
	auto end_node = m_model_desc->bone_node_address_array[ik.target_idx];
	positions.emplace_back(XMLoadFloat3(&end_node->start_pos));

	// ���ԋy�у��[�g�{�[��
	for (auto& chain_bone_idx : ik.node_idxes)
	{
		auto bone_node = m_model_desc->bone_node_address_array[chain_bone_idx];
		positions.emplace_back(XMLoadFloat3(&bone_node->start_pos));
	}

	//������Â炢�̂ŋt�ɂ���
	std::reverse(positions.begin(), positions.end());

	// ���̒�����}���Ă���
	edge_lens[0] = XMVector3Length(XMVectorSubtract(positions[1], positions[0])).m128_f32[0];
	edge_lens[1] = XMVector3Length(XMVectorSubtract(positions[2], positions[1])).m128_f32[0];

	// ���[�g�{�[�����W�ϊ�(�t���ɂȂ��Ă��邽�ߎg�p����C���f�b�N�X�ɒ���)
	positions[0] = XMVector3Transform(
		positions[0],
		m_map_buffers->bones[ik.node_idxes[1]]
	);

	// �^�񒆂͎����v�Z�����̂Ōv�Z���Ȃ�
	// ��[�{�[��
	positions[2] = XMVector3Transform(
		positions[2], 
		m_map_buffers->bones[ik.bone_idx]
	);

	// ���[�g�����[�ւ̃x�N�g��������Ă���
	auto linear_vec = XMVectorSubtract(positions[2], positions[0]);
	float a = XMVector3Length(linear_vec).m128_f32[0];
	float b = edge_lens[0];
	float c = edge_lens[1];

	linear_vec = XMVector3Normalize(linear_vec);

	// ���[�g����^�񒆂ւ̊p�x�v�Z
	float theta1 = acosf((a * a + b * b - c * c) / (2 * a * b));
	// �^�񒆂���^�[�Q�b�g�ւ̊p�x�v�Z
	float theta2 = acosf((b * b + c * c - a * a) / (2 * b * c));

	// �u���v�����߂�
	// �����^�񒆂��u�Ђ��v�ł������ꍇ�ɂ͋����I��X���Ƃ���
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
	else // �Ђ��̏ꍇ
	{
		// �E
		axis = XMVectorSet(1.f, 0.f, 0.f, 0.f);
	}

	// IK�`�F�[���̓��[�g�Ɍ������Ă��琔�����邽�߂P�����[�g�ɋ߂�
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

// LookAt�s��ɂ��{�[������������
void PMD_Actor::SolveLookAt(const PMD::IK& ik) noexcept
{
	using namespace DirectX;
	// ���̊֐��ɗ������_�Ńm�[�h�͂P�����Ȃ�
	// �`�F�[���ɓ����Ă���m�[�h�ԍ���IK�̃��[�g�m�[�h�̕��Ȃ̂ŁA
	// ���̃��[�g�m�[�h���疖�[�Ɍ������x�N�g�����l����

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
#include "../../Hrd/Debug.hpp"
#include <Helper/Math.hpp>

const std::vector<DirectX::XMFLOAT3> DebugManager::Cube::ModelVertex =
{
	{ -0.5f, -0.5f, -0.5f },	// ������
	{ -0.5f, +0.5f, -0.5f },	// ���㉜
	{ -0.5f, -0.5f, +0.5f },	// �����O
	{ -0.5f, +0.5f, +0.5f },	// ����O
	{ +0.5f, -0.5f, -0.5f },	// �E����
	{ +0.5f, +0.5f, -0.5f },	// �E�㉜
	{ +0.5f, -0.5f, +0.5f },	// �E���O
	{ +0.5f, +0.5f, +0.5f },	// �E��O
};

size_t DebugManager::Cube::GetVertex(DebugManager::Vertex* out) const
{
	if (!out)
	{
		return 0u;
	}

	Vertex v;
	v.color = color;

	using namespace DirectX;
	const XMMATRIX world = KGL::CreateWorldMatrix(pos, scale, rotate);

	size_t i = 0u;
	for (const auto& it : ModelVertex)
	{
		XMVECTOR local_pos = XMLoadFloat3(&it);
		XMStoreFloat3(&v.pos, XMVector3Transform(local_pos, world));
		out[i] = v;
		i++;
	}
	return i;
}
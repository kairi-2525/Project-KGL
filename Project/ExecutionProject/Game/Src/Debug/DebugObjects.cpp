#include "../../Hrd/Debug.hpp"
#include <Helper/Math.hpp>

const std::vector<DirectX::XMFLOAT3> DebugManager::Cube::ModelVertex =
{
	{ -0.5f, -0.5f, -0.5f },	// 左下奥
	{ -0.5f, +0.5f, -0.5f },	// 左上奥
	{ -0.5f, -0.5f, +0.5f },	// 左下前
	{ -0.5f, +0.5f, +0.5f },	// 左上前
	{ +0.5f, -0.5f, -0.5f },	// 右下奥
	{ +0.5f, +0.5f, -0.5f },	// 右上奥
	{ +0.5f, -0.5f, +0.5f },	// 右下前
	{ +0.5f, +0.5f, +0.5f },	// 右上前
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
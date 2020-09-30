#include "../../Hrd/Debug.hpp"
#include <Helper/Math.hpp>

static const DirectX::XMFLOAT3 CUBE_LDB = { -0.5f, -0.5f, -0.5f };	// ç∂â∫âú
static const DirectX::XMFLOAT3 CUBE_LUB = { -0.5f, +0.5f, -0.5f };	// ç∂â∫âú
static const DirectX::XMFLOAT3 CUBE_LDF = { -0.5f, -0.5f, +0.5f };	// ç∂â∫âú
static const DirectX::XMFLOAT3 CUBE_LUF = { -0.5f, +0.5f, +0.5f };	// ç∂â∫âú
static const DirectX::XMFLOAT3 CUBE_RDB = { +0.5f, -0.5f, -0.5f };	// ç∂â∫âú
static const DirectX::XMFLOAT3 CUBE_RUB = { +0.5f, +0.5f, -0.5f };	// ç∂â∫âú
static const DirectX::XMFLOAT3 CUBE_RDF = { +0.5f, -0.5f, +0.5f };	// ç∂â∫âú
static const DirectX::XMFLOAT3 CUBE_RUF = { +0.5f, +0.5f, +0.5f };	// ç∂â∫âú

static const DirectX::XMFLOAT3 NORMAL_F = { +0.f, +0.f, -1.f };
static const DirectX::XMFLOAT3 NORMAL_B = { +0.f, +0.f, +1.f };
static const DirectX::XMFLOAT3 NORMAL_U = { +0.f, +1.f, +0.f };
static const DirectX::XMFLOAT3 NORMAL_D = { +0.f, -1.f, +0.f };
static const DirectX::XMFLOAT3 NORMAL_R = { +1.f, +0.f, +0.f };
static const DirectX::XMFLOAT3 NORMAL_L = { -1.f, +0.f, +0.f };

const std::vector<DebugManager::Vertex> DebugManager::Cube::ModelVertex =
{
	// ëOñ 
	{ CUBE_LUF, NORMAL_F }, { CUBE_RUF, NORMAL_F }, { CUBE_LDF, NORMAL_F },
	{ CUBE_LDF, NORMAL_F }, { CUBE_RUF, NORMAL_F }, { CUBE_RDF, NORMAL_F },

	// îwñ 
	{ CUBE_LDB, NORMAL_B }, { CUBE_RUB, NORMAL_B }, { CUBE_LUB, NORMAL_B },
	{ CUBE_RDB, NORMAL_B }, { CUBE_RUB, NORMAL_B }, { CUBE_LDB, NORMAL_B },

	// âEñ 
	{ CUBE_RUF, NORMAL_R }, { CUBE_RUB, NORMAL_R }, { CUBE_RDF, NORMAL_R },
	{ CUBE_RDF, NORMAL_R }, { CUBE_RUB, NORMAL_R }, { CUBE_RDB, NORMAL_R },

	// ç∂ñ 
	{ CUBE_LDF, NORMAL_L }, { CUBE_LUB, NORMAL_L }, { CUBE_LUF, NORMAL_L },
	{ CUBE_LDB, NORMAL_L }, { CUBE_LUB, NORMAL_L }, { CUBE_LDF, NORMAL_L },

	// è„ñ 
	{ CUBE_LUB, NORMAL_U }, { CUBE_RUB, NORMAL_U }, { CUBE_LUF, NORMAL_U },
	{ CUBE_LUF, NORMAL_U }, { CUBE_RUB, NORMAL_U }, { CUBE_RUF, NORMAL_U },

	// â∫ñ 
	{ CUBE_LDF, NORMAL_D }, { CUBE_RDB, NORMAL_D }, { CUBE_LDB, NORMAL_D },
	{ CUBE_RDF, NORMAL_D }, { CUBE_RDB, NORMAL_D }, { CUBE_LDF, NORMAL_D },
};

size_t DebugManager::Cube::GetVertex(DebugManager::Vertex* out) const
{
	if (!out)
	{
		return 0u;
	}

	Vertex v;
	//v.color = color;

	using namespace DirectX;
	const XMMATRIX world = KGL::CreateWorldMatrix(pos, scale, rotate);

	size_t i = 0u;
	for (const auto& it : ModelVertex)
	{
		XMVECTOR local_pos = XMLoadFloat3(&it.pos);
		XMStoreFloat3(&v.pos, XMVector3Transform(local_pos, world));
		XMVECTOR local_normal = XMVectorSet(it.normal.x, it.normal.y, it.normal.z, 0.f);
		XMStoreFloat3(&v.normal, XMVector4Transform(local_normal, world));
		out[i] = v;
		i++;
	}
	return i;
}
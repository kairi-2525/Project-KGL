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

static const DirectX::XMFLOAT2 TEXCOORD_LU = { 0.f, 1.f };
static const DirectX::XMFLOAT2 TEXCOORD_RU = { 1.f, 1.f };
static const DirectX::XMFLOAT2 TEXCOORD_LD = { 0.f, 0.f };
static const DirectX::XMFLOAT2 TEXCOORD_RD = { 1.f, 0.f };

const std::vector<DebugManager::Vertex> DebugManager::Cube::ModelVertex =
{
	// ëOñ 
	{ CUBE_LUF, NORMAL_F, TEXCOORD_RU }, { CUBE_RUF, NORMAL_F, TEXCOORD_LU }, { CUBE_LDF, NORMAL_F, TEXCOORD_RD },
	{ CUBE_LDF, NORMAL_F, TEXCOORD_RD }, { CUBE_RUF, NORMAL_F, TEXCOORD_LU }, { CUBE_RDF, NORMAL_F, TEXCOORD_LD },

	// îwñ 
	{ CUBE_LDB, NORMAL_B, TEXCOORD_LD }, { CUBE_RUB, NORMAL_B, TEXCOORD_RU }, { CUBE_LUB, NORMAL_B, TEXCOORD_LU },
	{ CUBE_RDB, NORMAL_B, TEXCOORD_RD }, { CUBE_RUB, NORMAL_B, TEXCOORD_RU }, { CUBE_LDB, NORMAL_B, TEXCOORD_LD },

	// âEñ 
	{ CUBE_RUF, NORMAL_R, TEXCOORD_RU }, { CUBE_RUB, NORMAL_R, TEXCOORD_LU }, { CUBE_RDF, NORMAL_R, TEXCOORD_RD },
	{ CUBE_RDF, NORMAL_R, TEXCOORD_RD }, { CUBE_RUB, NORMAL_R, TEXCOORD_LU }, { CUBE_RDB, NORMAL_R, TEXCOORD_LD },

	// ç∂ñ 
	{ CUBE_LDF, NORMAL_L, TEXCOORD_LD }, { CUBE_LUB, NORMAL_L, TEXCOORD_RU }, { CUBE_LUF, NORMAL_L, TEXCOORD_LU },
	{ CUBE_LDB, NORMAL_L, TEXCOORD_RD }, { CUBE_LUB, NORMAL_L, TEXCOORD_RU }, { CUBE_LDF, NORMAL_L, TEXCOORD_LD },

	// è„ñ 
	{ CUBE_LUB, NORMAL_U, TEXCOORD_RU }, { CUBE_RUB, NORMAL_U, TEXCOORD_LU }, { CUBE_LUF, NORMAL_U, TEXCOORD_RD },
	{ CUBE_LUF, NORMAL_U, TEXCOORD_RD }, { CUBE_RUB, NORMAL_U, TEXCOORD_LU }, { CUBE_RUF, NORMAL_U, TEXCOORD_LD },

	// â∫ñ 
	{ CUBE_LDF, NORMAL_D, TEXCOORD_LD }, { CUBE_RDB, NORMAL_D, TEXCOORD_RU }, { CUBE_LDB, NORMAL_D, TEXCOORD_LU },
	{ CUBE_RDF, NORMAL_D, TEXCOORD_RD }, { CUBE_RDB, NORMAL_D, TEXCOORD_RU }, { CUBE_LDF, NORMAL_D, TEXCOORD_LD },
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
		//v.bitangent = v.tangent = v.normal;
		v.texcoord = it.texcoord;
		out[i] = v;
		i++;
	}
	return i;
}
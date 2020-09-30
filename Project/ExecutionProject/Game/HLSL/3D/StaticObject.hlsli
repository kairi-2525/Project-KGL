// https://github.com/Nadrin/PBR/blob/master/data/shaders/hlsl/pbr.hlsl
//物理ベースのレンダリング

//物理ベースのシェーディングモデル：Lambetrtian拡散BRDF + Cook-TorranceマイクロファセットスペキュラーBRDF +アンビエント用IBL。

//この実装は、EpicGamesによる「UnrealEngine4のリアルシェーディング」SIGGRAPH2013コースノートに基づいています。
//参照：http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf

static const float PI = 3.141592;
static const float Epsilon = 0.00001;

static const uint NumLights = 3;

// すべての誘電体の一定の垂直入射フレネル係数。
static const float3 Fdielectric = 0.04;

cbuffer TransformConstants : register(b0)
{
	row_major matrix viewProjectionMatrix;
	row_major matrix skyProjectionMatrix;
};

cbuffer ShadingConstants : register(b1)
{
	struct {
		float3 direction;
		float3 radiance;
	} lights[NumLights];
	float3 eyePosition;
};

struct VSInput
{
	float3 position  : POSITION;
	float3 normal    : NORMAL;
	float3 tangent   : TANGENT;
	float3 bitangent : BITANGENT;
	float2 texcoord  : TEXCOORD;
};

struct PSInput
{
	float4 pixel_position : SV_POSITION;
	float3 position : POSITION;
	float2 texcoord : TEXCOORD;
	float3x3 tangent_basis : TBASIS;
};

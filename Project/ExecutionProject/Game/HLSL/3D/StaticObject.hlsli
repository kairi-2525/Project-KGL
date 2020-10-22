// https://github.com/Nadrin/PBR/blob/master/data/shaders/hlsl/pbr.hlsl

static const float PI = 3.141592;
static const float Epsilon = 0.00001;

static const uint NumLights = 3;

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
	float2 texcoord  : TEXCOORD;
};

struct GSInput
{
	float4 pixel_position  : SV_POSITION;
	float3 position : POSITION;
	float3 normal    : NORMAL;
	float2 texcoord  : TEXCOORD;
};

struct PSInput
{
	float4 pixel_position : SV_POSITION;
	float3 position : POSITION;
	float2 texcoord : TEXCOORD;
	float3x3 tangent_basis : TBASIS;
};

// https://github.com/Nadrin/PBR/blob/master/data/shaders/hlsl/pbr.hlsl
//$BJ*M}%Y!<%9$N%l%s%@%j%s%0(B

//$BJ*M}%Y!<%9$N%7%'!<%G%#%s%0%b%G%k!'(BLambetrtian$B3H;6(BBRDF + Cook-Torrance$B%^%$%/%m%U%!%;%C%H%9%Z%-%e%i!<(BBRDF +$B%"%s%S%(%s%HMQ(BIBL$B!#(B

//$B$3$N<BAu$O!"(BEpicGames$B$K$h$k!V(BUnrealEngine4$B$N%j%"%k%7%'!<%G%#%s%0!W(BSIGGRAPH2013$B%3!<%9%N!<%H$K4p$E$$$F$$$^$9!#(B
//$B;2>H!'(Bhttp://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf

static const float PI = 3.141592;
static const float Epsilon = 0.00001;

static const uint NumLights = 3;

// $B$9$Y$F$NM6EEBN$N0lDj$N?bD>F~<M%U%l%M%k78?t!#(B
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

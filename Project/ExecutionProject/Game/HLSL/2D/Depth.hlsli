Texture2D<float> depth_tex : register (t0);
SamplerState smp : register (s0);

struct PSInput
{
	float4 svpos : SV_POSITION;
	float2 uv : TEXCOORD;
};
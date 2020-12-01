#include "StaticModel.hlsli"

cbuffer MaterialBuffer : register(b2)
{
	float3 color;
};

Texture2D<float4> ambient_tex				: register(t0);
Texture2D<float4> diffuse_tex				: register(t1);
Texture2D<float4> specular_tex				: register(t2);
Texture2D<float4> specular_highlights_tex	: register(t3);
Texture2D<float4> dissolve_tex				: register(t4);
Texture2D<float4> bump_tex					: register(t5);
Texture2D<float4> displacement_tex			: register(t6);
Texture2D<float4> stencil_decal_tex			: register(t7);

SamplerState smp : register(s0);

float4 PSMain(PS_Input input) : SV_TARGET
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
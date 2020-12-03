#include "StaticModel.hlsli"

static const float PI = 3.141592;

cbuffer MaterialBuffer : register(b2)
{
	float3			ambient_color;
	float3			diffuse_color;
	float3			specular_color;
	float			specular_weight;
	bool			specular_flg;
	float			dissolve;	// 透明度 1なら透明
	float			refraction;	// 屈折率
	bool			smooth;
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

float3 fresnel_schlick(float NoL, float3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - NoL, 5.8);
}

float4 PSMain(PS_Input input) : SV_TARGET
{
	// 接線行列を作成
	row_major float3x3 tanget_mat =
	{
		input.normal,
		input.tangent,
		input.bitangent
	};

	// ライト方向が反転しているように見える???
	float3 L = -light_vec;
	L = normalize(mul(L, tanget_mat));
	float3 N = normalize((bump_tex.Sample(smp, input.uv).rgb * 2.f) - 1.f);
	N = float3(0.f, 1.f, 0.f);
	if (smooth)
	{
		N = input.normal;
		L = -light_vec;
	}

	float3 ambient = ambient_tex.Sample(smp, input.uv).rgb * ambient_color;

	float s_irraduabce = max(0, dot(N, L));
	float3 irraduabce = light_radiance * s_irraduabce + ambient;

	// 反射率
	float reflectance = specular_flg ? fresnel_schlick(s_irraduabce, 0.4f) * specular_weight : 0.f;
	float3 diffuse = diffuse_tex.Sample(smp, input.uv).rgb * diffuse_color;

	float3 diffuse_exitance = diffuse * irraduabce * (1.f - reflectance);
	float3 diffuse_radiance = diffuse_exitance / PI;

	float3 R = normalize(reflect(-L, N));
	float3 V = normalize(eye_pos - input.position);

	float3 specular = specular_tex.Sample(smp, input.uv).rgb * specular_color;

	float3 specular_exitance = specular * irraduabce * reflectance;
	float3 specular_radiance = specular_exitance * pow(max(0, dot(V, R)), 36);

	float3 radiance = diffuse_radiance + specular_radiance;

	return float4(radiance, 1.f - dissolve);
}
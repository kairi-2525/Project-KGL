#include "StaticModel.hlsli"

static const float PI = 3.141592;

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
	float3 L = -light_vec;

	float3 N = (float3)0;
	if (smooth)
	{
		// ê⁄ê¸çsóÒÇçÏê¨
		float3x3 tanget_mat =
		{
			input.tangent,
			input.bitangent,
			input.normal
		};

		N = bump_tex.Sample(smp, input.uv).rgb;
		N = (N * 2.f) - 1.f;
		N = mul(N, tanget_mat);
		N = normalize(N);
	}
	else
	{
		N = input.normal;
	}

	float3 ambient = ambient_tex.Sample(smp, input.uv).rgb * (ambient_color * ambient_light_color);

	float light_intensity = max(0, dot(N, L));
	float3 irraduabce = (light_color * light_intensity) + ambient;

	// îΩéÀó¶
	float reflectance = specular_flg ? fresnel_schlick(light_intensity, 0.4f) * specular_weight : 0.f;
	
	float4 col = diffuse_tex.Sample(smp, input.uv);
	float3 diffuse = col.rgb * diffuse_color;

	float3 diffuse_exitance = diffuse * irraduabce * (1.f - reflectance);
	float3 diffuse_radiance = diffuse_exitance / PI;

	float3 R = normalize(reflect(-L, N));
	float3 V = normalize(eye_pos - input.position);

	float3 specular = specular_tex.Sample(smp, input.uv).rgb * specular_color;

	float3 specular_exitance = specular * irraduabce * reflectance;
	float3 specular_radiance = specular_exitance * pow(max(0, dot(V, R)), 36);

	float3 radiance = diffuse_radiance + specular_radiance;

	return float4(radiance, col.a * dissolve);
}
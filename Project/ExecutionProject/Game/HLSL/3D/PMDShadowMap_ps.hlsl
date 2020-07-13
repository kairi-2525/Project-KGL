#include "PMDShaderHeader.hlsli"

Texture2D<float> light_depth_tex : register(t4);

float4 PSMain(Output input) : SV_TARGET
{
	float3 light = normalize(float3(1, -1, 1));
	float3 light_color = float3(1, 1, 1);

	// 輝度
	float brightness = dot(-light, input.normal.xyz);	

	float4 toon_dif = material_toon.Sample(toom_smp, float2(0, 1.0 - brightness));

	// 光の反射ベクトル
	float3 ref_light = normalize(reflect(light, input.normal.xyz));
	float specular_b = pow(saturate(dot(ref_light, -input.ray)), specular.a);

	// スフィアマップ用uv
	float2 sphere_map_uv = (input.vnormal.xy + float2(1, -1)) * float2(0.5, -0.5);

	// テクスチャカラー
	float4 tex_color = material_tex.Sample(smp, input.uv);

	//return float4(depth_light, depth_light, depth_light, 1.f);

	float4 ret = 
		max(saturate(toon_dif
			* diffuse
			* tex_color
			* material_sph.Sample(smp, sphere_map_uv)
			+ material_spa.Sample(smp, sphere_map_uv) * tex_color
			+ float4(specular_b * specular.rgb, 1))
			, float4(tex_color.xyz * ambient, 1)
		);

	float3 light_vp_pos = input.tpos.xyz / input.tpos.w;
	float2 shadow_uv = (light_vp_pos.xy + float2(1, -1) * float2(0.5f, 0.5f));

	float depth_light = light_depth_tex.Sample(smp, shadow_uv);
	float shadow_weight = 1.f;
	if (depth_light < input.tpos.z)
	{
		shadow_weight = 0.5f;
	}

	return float4(ret.rgb * shadow_weight, ret.a);
}
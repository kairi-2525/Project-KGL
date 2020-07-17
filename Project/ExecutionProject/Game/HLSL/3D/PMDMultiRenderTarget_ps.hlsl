#include "PMDShaderHeader.hlsli"

Texture2D<float> light_depth_tex : register(t4);
SamplerComparisonState shadow_smp : register(s2);

struct PSOut
{
	float4 col : SV_TARGET0;
	float4 normal : SV_TARGET1;
	float4 pos : SV_TARGET2;
};

PSOut PSMain(Output input)
{
	PSOut output = (PSOut)0;

	float3 light_color = float3(1, 1, 1);

	// 輝度
	float brightness = dot(-light_vec, input.normal.xyz);

	float4 toon_dif = material_toon.Sample(toom_smp, float2(0, 1.0 - brightness));

	// 光の反射ベクトル
	float3 ref_light = normalize(reflect(light_vec, input.normal.xyz));
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
	float2 shadow_uv = (light_vp_pos.xy + float2(1, -1)) * float2(0.5f, -0.5f);

	float depth_light = light_depth_tex.SampleCmp(shadow_smp, shadow_uv, light_vp_pos.z - 0.0015f);
	float shadow_weight = lerp(0.5f, 1.0f, depth_light); // depth_lightが 0 の時 0.5, 1のとき 1.0になる。

	output.col = float4(ret.rgb * shadow_weight, ret.a);

	output.normal.xyz = input.normal.xyz;
	output.normal.w = 1.0f;
	output.pos = float4(input.pos.xyz, 1.f);

	return output;
}
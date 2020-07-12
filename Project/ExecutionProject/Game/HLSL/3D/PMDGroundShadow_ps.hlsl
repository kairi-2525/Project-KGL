#include "PMDGroundShadow.hlsli"

float4 PSMain(Output input) : SV_TARGET
{
	if (input.inst_no == 1)
	{
		return float4(0.f, 0.f, 0.f, 1.f);
	}

	float3 light = normalize(float3(1, -1, 1));
	float3 light_color = float3(1, 1, 1);

	// �P�x
	float brightness = dot(-light, input.normal.xyz);

	float4 toon_dif = material_toon.Sample(toom_smp, float2(0, 1.0 - brightness));

	// ���̔��˃x�N�g��
	float3 ref_light = normalize(reflect(light, input.normal.xyz));
	float specular_b = pow(saturate(dot(ref_light, -input.ray)), specular.a);

	// �X�t�B�A�}�b�v�puv
	float2 sphere_map_uv = (input.vnormal.xy + float2(1, -1)) * float2(0.5, -0.5);

	// �e�N�X�`���J���[
	float4 tex_color = material_tex.Sample(smp, input.uv);

	return
		max(saturate(toon_dif
			* diffuse
			* tex_color
			* material_sph.Sample(smp, sphere_map_uv)
			+ material_spa.Sample(smp, sphere_map_uv) * tex_color
			+ float4(specular_b * specular.rgb, 1))
			, float4(tex_color.xyz * ambient, 1)
		);
}
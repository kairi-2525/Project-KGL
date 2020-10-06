#include "StaticObject.hlsli"

Texture2D albedo_texture : register(t0);
Texture2D normal_texture : register(t1);
Texture2D metalness_texture : register(t2);
Texture2D roughness_texture : register(t3);
Texture2D specular_texture : register(t4);
Texture2D irradiance_texture : register(t5);
//TextureCube specular_texture : register(t4);
//TextureCube irradiance_texture : register(t5);
Texture2D specular_brdf_lut : register(t6);

SamplerState default_sampler : register(s0);
SamplerState sp_brdf_sampler : register(s1);

// GGX / Towbridge-Reitz���K���z�֐��B
// �f�B�Y�j�[�̃A���t�@=�e��^ 2�̍ăp�����[�^�����g�p���܂��B
float ndfGGX(float cosLh, float roughness)
{
	float alpha = roughness * roughness;
	float alphaSq = alpha * alpha;

	float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}

// �ȉ��̕����\��Schlick-GGX�̒P��p��B
float gaSchlickG1(float cosTheta, float k)
{
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

// �X�~�X�̕��@���g�p�����􉽊w�I�����֐���Schlick-GGX�ߎ��B
float gaSchlickGGX(float cosLi, float cosLo, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
	return gaSchlickG1(cosLi, k) * gaSchlickG1(cosLo, k);
}

// �t���l���W����Shlick�̋ߎ��B
float3 fresnelSchlick(float3 F0, float cosTheta)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// �X�y�L�����[IBL���}�b�v�̃~�b�v�}�b�v���x���̐���Ԃ��܂��B
uint querySpecularTextureLevels()
{
	uint width, height, levels;
	specular_texture.GetDimensions(0, width, height, levels);
	return levels;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	//// ���̓e�N�X�`�����T���v�����O���āA�V�F�[�f�B���O���f���̃p�����[�^���擾���܂��B
	//float3 albedo = albedo_texture.Sample(default_sampler, input.texcoord).rgb;
	//float metalness = metalness_texture.Sample(default_sampler, input.texcoord).r;
	//float roughness = roughness_texture.Sample(default_sampler, input.texcoord).r;

	//// ���M���̕����i���[���h�X�y�[�X�t���O�����g�̈ʒu����u�ځv�ւ̃x�N�g���j�B
	//float3 Lo = normalize(eyePosition - input.position);
	//// ���݂̃t���O�����g�̖@�����擾���A���[���h�X�y�[�X�ɕϊ����܂��B
	//float3 N = normalize(2.0 * normal_texture.Sample(default_sampler, input.texcoord).rgb - 1.0);
	//N = normalize(mul(input.tangent_basis, N));

	//// �\�ʂ̖@�������ƌ��̕����̊Ԃ̊p�x�B
	//float cosLo = max(0.0, dot(N, Lo));

	//// ���ʔ���(Specular)�x�N�g���B
	//float3 Lr = 2.0 * cosLo * N - Lo;

	//// �@�����˂ł̃t���l�����˗��i�����̏ꍇ�̓A���x�h�J���[���g�p�j�B
	//float3 F0 = lerp(Fdielectric, albedo, metalness);

	//// ���̓��C�g�̒��ڏƖ��v�Z�B
	//float3 directLighting = 0.0;
	//for (uint i = 0; i < NumLights; ++i)
	//{
	//	float3 Li = -lights[i].direction;
	//	float3 Lradiance = lights[i].radiance;

	//	// Li��Lo�̊Ԃ̃n�[�t�x�N�g���B
	//	float3 Lh = normalize(Li + Lo);

	//	// �\�ʖ@���Ƃ��܂��܂Ȍ��x�N�g���̊Ԃ̊p�x���v�Z���܂��B
	//	float cosLi = max(0.0, dot(N, Li));
	//	float cosLh = max(0.0, dot(N, Lh));

	//	// ���ڏƖ��̃t���l�������v�Z���܂��B
	//	float3 F = fresnelSchlick(F0, max(0.0, dot(Lh, Lo)));
	//	// ����BRDF�̐��K���z���v�Z���܂��B
	//	float D = ndfGGX(cosLh, roughness);
	//	// ����BRDF�̊􉽊w�I�������v�Z���܂��B
	//	float G = gaSchlickGGX(cosLi, cosLo, roughness);

	//	// �U���U���́A�����U�d�̔}�̂ɂ���ĕ�������܂���邽�߂ɔ������܂��B
	//	// ����A�����̓G�l���M�[�𔽎˂܂��͋z�����邽�߁A�g�U�̊�^�͏�Ƀ[���ł��B 
	//	// �G�l���M�[��ߖ񂷂�ɂ́A�t���l���W���Ƌ������Ɋ�Â��Ċg�UBRDF��^���X�P�[�����O����K�v������܂��B
	//	float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), metalness);

	//	// �����o�[�g�g�UBRDF�B �Ɩ��ƃ}�e���A���̒P�ʂ����֗��ɂ��邽�߂ɁA1 / PI�ŃX�P�[�����O���܂���B
	//	// �Q�� : https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
	//	float3 diffuseBRDF = kd * albedo;

	//	// Cook-Torrance�X�y�L�����[�}�C�N���t�@�Z�b�gBRDF�B
	//	float3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * cosLo);

	//	// ���̃��C�g�ւ̑��v���B
	//	directLighting += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;
	//}

	//// �A���r�G���g�Ɩ��iIBL�j�B
	//float3 ambientLighting;
	//{
	//	// �@�������̃T���v���g�U���ˏƓx�B
	//	float3 irradiance = irradiance_texture.Sample(default_sampler, N).rgb;

	//	// ���͏Ɩ��̃t���l�������v�Z���܂��B 
	//	// ���O�Ƀt�B���^�����O���ꂽ�L���[�u�}�b�v���g�p���A���ˏƓx�͑����̕������痈�邽�߁A
	//	// ���̔��x�N�g���i��L��cosLh�j�Ƃ̊p�x�̑����cosLo���g�p���܂��B 
	//	// �Q�� �F https://seblagarde.wordpress.com/2011/08/17/hello-world/
	//	float3 F = fresnelSchlick(F0, cosLo);

	//	// �g�U��^�W�����擾���܂��i���ڏƖ��̏ꍇ�Ɠ��l�j�B
	//	float3 kd = lerp(1.0 - F, 0.0, metalness);

	//	// ���ˏƓx�}�b�v�ɂ́A�����o�[�gBRDF��z�肵�������̕��ˋP�x���܂܂�Ă��܂��B
	//	// �����ł��A1 / PI�ŃX�P�[�����O����K�v�͂���܂���B
	//	float3 diffuseIBL = kd * albedo * irradiance;

	//	// �������~�b�v�}�b�v���x���ł̏\���Ȏ��O�t�B���^�����O���ꂽ���ʔ��ˊ��B
	//	uint specularTextureLevels = querySpecularTextureLevels();
	//	float3 specularIrradiance = specular_texture.SampleLevel(default_sampler, Lr, roughness * specularTextureLevels).rgb;

	//	// Cook-Torrance����BRDF�̕����a�ߎ��W���B
	//	//float2 specularBRDF = specular_brdf_lut.Sample(sp_brdf_sampler, float2(cosLo, roughness)).rg;
	//	float2 specularBRDF = specular_texture.Sample(sp_brdf_sampler, float2(cosLo, roughness)).rg;

	//	// ���ʔ���IBL�̊�^�̍��v�B
	//	float3 specularIBL = (F0 * specularBRDF.x + specularBRDF.y) * specularIrradiance;

	//	// �����͏Ɩ��̊�^�B
	//	ambientLighting = diffuseIBL + specularIBL;
	//}

	////return float4(1.f, 1.f, 1.f, 1.f);
	//// �ŏI�I�ȃt���O�����g�̐F�B
	//return float4(directLighting + ambientLighting, 1.0);

	//return float4(1.0f, 1.0f, 1.0f, 1.0f);


	return float4(albedo_texture.Sample(default_sampler, input.texcoord).rgb, 1.0);
}
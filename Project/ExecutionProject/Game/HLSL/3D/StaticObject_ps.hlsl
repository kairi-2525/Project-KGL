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

// GGX / Towbridge-Reitz正規分布関数。
// ディズニーのアルファ=粗さ^ 2の再パラメータ化を使用します。
float ndfGGX(float cosLh, float roughness)
{
	float alpha = roughness * roughness;
	float alphaSq = alpha * alpha;

	float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}

// 以下の分離可能なSchlick-GGXの単一用語。
float gaSchlickG1(float cosTheta, float k)
{
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

// スミスの方法を使用した幾何学的減衰関数のSchlick-GGX近似。
float gaSchlickGGX(float cosLi, float cosLo, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
	return gaSchlickG1(cosLi, k) * gaSchlickG1(cosLo, k);
}

// フレネル係数のShlickの近似。
float3 fresnelSchlick(float3 F0, float cosTheta)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// スペキュラーIBL環境マップのミップマップレベルの数を返します。
uint querySpecularTextureLevels()
{
	uint width, height, levels;
	specular_texture.GetDimensions(0, width, height, levels);
	return levels;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	//// 入力テクスチャをサンプリングして、シェーディングモデルのパラメータを取得します。
	//float3 albedo = albedo_texture.Sample(default_sampler, input.texcoord).rgb;
	//float metalness = metalness_texture.Sample(default_sampler, input.texcoord).r;
	//float roughness = roughness_texture.Sample(default_sampler, input.texcoord).r;

	//// 発信光の方向（ワールドスペースフラグメントの位置から「目」へのベクトル）。
	//float3 Lo = normalize(eyePosition - input.position);
	//// 現在のフラグメントの法線を取得し、ワールドスペースに変換します。
	//float3 N = normalize(2.0 * normal_texture.Sample(default_sampler, input.texcoord).rgb - 1.0);
	//N = normalize(mul(input.tangent_basis, N));

	//// 表面の法線方向と光の方向の間の角度。
	//float cosLo = max(0.0, dot(N, Lo));

	//// 鏡面反射(Specular)ベクトル。
	//float3 Lr = 2.0 * cosLo * N - Lo;

	//// 法線入射でのフレネル反射率（金属の場合はアルベドカラーを使用）。
	//float3 F0 = lerp(Fdielectric, albedo, metalness);

	//// 分析ライトの直接照明計算。
	//float3 directLighting = 0.0;
	//for (uint i = 0; i < NumLights; ++i)
	//{
	//	float3 Li = -lights[i].direction;
	//	float3 Lradiance = lights[i].radiance;

	//	// LiとLoの間のハーフベクトル。
	//	float3 Lh = normalize(Li + Lo);

	//	// 表面法線とさまざまな光ベクトルの間の角度を計算します。
	//	float cosLi = max(0.0, dot(N, Li));
	//	float cosLh = max(0.0, dot(N, Lh));

	//	// 直接照明のフレネル項を計算します。
	//	float3 F = fresnelSchlick(F0, max(0.0, dot(Lh, Lo)));
	//	// 鏡面BRDFの正規分布を計算します。
	//	float D = ndfGGX(cosLh, roughness);
	//	// 鏡面BRDFの幾何学的減衰を計算します。
	//	float G = gaSchlickGGX(cosLi, cosLo, roughness);

	//	// 散漫散乱は、光が誘電体媒体によって複数回屈折されるために発生します。
	//	// 一方、金属はエネルギーを反射または吸収するため、拡散の寄与は常にゼロです。 
	//	// エネルギーを節約するには、フレネル係数と金属性に基づいて拡散BRDF寄与をスケーリングする必要があります。
	//	float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), metalness);

	//	// ランバート拡散BRDF。 照明とマテリアルの単位をより便利にするために、1 / PIでスケーリングしません。
	//	// 参照 : https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
	//	float3 diffuseBRDF = kd * albedo;

	//	// Cook-TorranceスペキュラーマイクロファセットBRDF。
	//	float3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * cosLo);

	//	// このライトへの総貢献。
	//	directLighting += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;
	//}

	//// アンビエント照明（IBL）。
	//float3 ambientLighting;
	//{
	//	// 法線方向のサンプル拡散放射照度。
	//	float3 irradiance = irradiance_texture.Sample(default_sampler, N).rgb;

	//	// 周囲照明のフレネル項を計算します。 
	//	// 事前にフィルタリングされたキューブマップを使用し、放射照度は多くの方向から来るため、
	//	// 光の半ベクトル（上記のcosLh）との角度の代わりにcosLoを使用します。 
	//	// 参照 ： https://seblagarde.wordpress.com/2011/08/17/hello-world/
	//	float3 F = fresnelSchlick(F0, cosLo);

	//	// 拡散寄与係数を取得します（直接照明の場合と同様）。
	//	float3 kd = lerp(1.0 - F, 0.0, metalness);

	//	// 放射照度マップには、ランバートBRDFを想定した既存の放射輝度が含まれています。
	//	// ここでも、1 / PIでスケーリングする必要はありません。
	//	float3 diffuseIBL = kd * albedo * irradiance;

	//	// 正しいミップマップレベルでの十分な事前フィルタリングされた鏡面反射環境。
	//	uint specularTextureLevels = querySpecularTextureLevels();
	//	float3 specularIrradiance = specular_texture.SampleLevel(default_sampler, Lr, roughness * specularTextureLevels).rgb;

	//	// Cook-Torrance鏡面BRDFの分割和近似係数。
	//	//float2 specularBRDF = specular_brdf_lut.Sample(sp_brdf_sampler, float2(cosLo, roughness)).rg;
	//	float2 specularBRDF = specular_texture.Sample(sp_brdf_sampler, float2(cosLo, roughness)).rg;

	//	// 鏡面反射IBLの寄与の合計。
	//	float3 specularIBL = (F0 * specularBRDF.x + specularBRDF.y) * specularIrradiance;

	//	// 総周囲照明の寄与。
	//	ambientLighting = diffuseIBL + specularIBL;
	//}

	////return float4(1.f, 1.f, 1.f, 1.f);
	//// 最終的なフラグメントの色。
	//return float4(directLighting + ambientLighting, 1.0);

	//return float4(1.0f, 1.0f, 1.0f, 1.0f);


	return float4(albedo_texture.Sample(default_sampler, input.texcoord).rgb, 1.0);
}
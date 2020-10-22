#include "StaticObject.hlsli"

GSInput VSMain(VSInput input)
{
	GSInput output = (GSInput)0;
	output.position = input.position;
	output.texcoord = float2(input.texcoord.x, 1.0 - input.texcoord.y);

	// 接空間基底ベクトルを渡します（法線マッピングの場合）。
	//float3x3 TBN = float3x3(input.tangent, input.bitangent, input.normal);
	//output.tangent_basis = mul(transpose(TBN), (row_major float3x3)sceneRotationMatrix);
	//output.tangent_basis = transpose(TBN);

	output.pixel_position = mul(float4(input.position, 1.f), viewProjectionMatrix);
	return output;
}
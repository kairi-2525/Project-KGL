#include "StaticModel.hlsli"

GS_Input VSMain(VS_Input input)
{
	return input;
	/*PS_Input output = (PS_Input)0;
	float4 pos = float4(input.position, 1.f);
	output.sv_position = mul(pos, wvp);
	output.position = mul(pos, world).rgb;
	output.normal = normalize(mul(input.normal, (float3x3)world));
	output.tangent = normalize(mul(input.tangent, (float3x3)world));
	output.bitangent = normalize(mul(input.bitangent, (float3x3)world));
	output.uv = input.uv;

	return output;*/
}
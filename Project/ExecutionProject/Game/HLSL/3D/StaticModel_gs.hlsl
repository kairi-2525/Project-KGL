#include "StaticModel.hlsli"

cbuffer ModelBuffer : register(b1)
{
	row_major matrix world;
	row_major matrix wvp;
};

[maxvertexcount(3)]
void GSMain(
	triangle GS_Input input[3],
	inout TriangleStream< PS_Input > output
)
{
	PS_Input element = (PS_Input)0;
	float3 flat_N = (float3)0;
	if (!smooth)
	{
		//ñ ñ@ê¸éZèo
		/*float3 v1 = input[1].position.xyz - input[0].position.xyz;
		float3 v2 = input[2].position.xyz - input[0].position.xyz;
		flat_N = normalize(cross(v1, v2));*/

		float3 v1 = input[1].position.xyz - input[0].position.xyz;
		float3 v2 = input[2].position.xyz - input[0].position.xyz;
		flat_N = normalize(cross(v1, v2));

		//flat_N = (input[0].normal + input[1].normal + input[2].normal) / 3;
		element.normal = normalize(mul(flat_N, (float3x3)world));
	}

	for (uint i = 0; i < 3; i++)
	{
		float4 pos = float4(input[i].position, 1.f);
		element.sv_position = mul(pos, wvp);
		element.position = mul(pos, world).rgb;
		element.uv = input[i].uv;

		if (smooth)
		{
			element.normal = normalize(mul(input[i].normal, (float3x3)world));
			element.tangent = normalize(mul(input[i].tangent, (float3x3)world));
			element.bitangent = normalize(mul(input[i].bitangent, (float3x3)world));
		}
		//else
		//{
		//	element.normal = normalize(mul(flat_N, (float3x3)world));
		//	/*float3 tangent = normalize(cross(flat_N, float3(0.f, 1.f, 0.f)));
		//	if (dot(tangent, tangent) < 1.f)
		//	{
		//		tangent = normalize(cross(flat_N, float3(1.f, 0.f, 0.f)));
		//	}
		//	element.tangent = normalize(mul(tangent, (float3x3)world));

		//	float3 bitangent = normalize(cross(tangent, flat_N));
		//	element.bitangent = normalize(mul(bitangent, (float3x3)world));*/
		//}

		output.Append(element);
	}
	output.RestartStrip();
}
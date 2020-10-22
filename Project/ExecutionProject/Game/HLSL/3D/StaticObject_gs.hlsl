#include "StaticObject.hlsli"

[maxvertexcount(3)]
void GSMain(
	triangle GSInput input[3],
	inout TriangleStream< PSInput > output
)
{
	PSInput element[3];
	element[0].pixel_position = input[0].pixel_position;
	element[0].position = input[0].position;
	element[0].texcoord = input[0].texcoord;

	element[1].pixel_position = input[1].pixel_position;
	element[1].position = input[1].position;
	element[1].texcoord = input[1].texcoord;

	element[2].pixel_position = input[2].pixel_position;
	element[2].position = input[2].position;
	element[2].texcoord = input[2].texcoord;

	// Edges of the triangle : postion delta
	float3 delta_pos0 = element[1].position - element[0].position;
	float3 delta_pos1 = element[2].position - element[0].position;

	// UV delta
	float2 delta_uv0 = element[1].texcoord - element[0].texcoord;
	float2 delta_uv1 = element[2].texcoord - element[0].texcoord;

	// 接線と従接線を計算
	float r = 1.0f / (delta_uv0.x * delta_uv1.y - delta_uv0.y * delta_uv1.x);
	float3 tangent = (delta_pos0 * delta_uv1.y - delta_pos1 * delta_uv0.y) * r;
	float3 bitangent = (delta_pos1 * delta_uv0.x - delta_pos0 * delta_uv1.x) * r;

	tangent = normalize(tangent);
	bitangent = normalize(bitangent);

	// 接空間基底ベクトルを渡します（法線マッピングの場合）。
	float3x3 TBN[3];
	TBN[0] = float3x3(tangent, bitangent, input[0].normal);
	TBN[1] = float3x3(tangent, bitangent, input[1].normal);
	TBN[2] = float3x3(tangent, bitangent, input[2].normal);

	// 回転しないので使わない↓
	//output.tangent_basis = mul(transpose(TBN), (row_major float3x3)viewProjectionMatrix);
	element[0].tangent_basis = transpose(TBN[0]);
	element[1].tangent_basis = transpose(TBN[1]);
	element[2].tangent_basis = transpose(TBN[2]);

	output.Append(element[0]);
	output.Append(element[1]);
	output.Append(element[2]);
}
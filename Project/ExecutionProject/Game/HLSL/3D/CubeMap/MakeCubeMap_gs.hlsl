#include "MakeCubeMap.hlsli"

[maxvertexcount(18)]
void GSMain(
	triangle GS_Input input[3],
	inout TriangleStream< PS_Input > output
)
{
	float unsmooth_normal;

	if (!smooth)
	{
		float3 flat_N = (float3)0;
		//�ʖ@���Z�o
		float3 v1 = input[1].position.xyz - input[0].position.xyz;
		float3 v2 = input[2].position.xyz - input[0].position.xyz;
		flat_N = normalize(cross(v1, v2));

		unsmooth_normal = normalize(mul(flat_N, (float3x3)world));
	}

	float3 normals[3];
	float3 tangents[3];
	float3 bitangents[3];
	float4 positions[3];

	for (uint i = 0; i < 3; i++)
	{
		positions[i] = mul(float4(input[i].position, 1.f), world);

		if (smooth)
		{
			normals[i] = normalize(mul(input[i].normal, (float3x3)world));
			tangents[i] = normalize(mul(input[i].tangent, (float3x3)world));
			bitangents[i] = normalize(mul(input[i].bitangent, (float3x3)world));
		}
	}

	for (uint i = 0; i < 6; i++)
	{
		PS_Input element = (PS_Input)0;
		element.rt_index = i;
		for (uint j = 0; j < 3; j++)
		{
			//�@�r���[�E���e�ϊ�
			float4 P = mul(view_mat[i], positions[j]);
			element.sv_position = mul(projection, P);
			element.position = positions[j].xyz;

			//�@���[���h�@��
			element.normal = normals[j];
			element.tangent = tangents[j];
			element.bitangent = bitangents[j];

			//�e�N�X�`���[���W
			element.uv = input[j].uv;

			output.Append(element);
		}
		output.RestartStrip();
	}
}
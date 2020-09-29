#include "StaticObject.hlsli"

struct VSInput
{
	float4 pos : POSITION;
	float4 color : COLOR;
};

PSInput main(VSInput input)
{
	PSInput output = (PSInput)0;
	output.sv_pos = mul(input.pos, view_proj);
	output.color = input.color;
	return output;
}
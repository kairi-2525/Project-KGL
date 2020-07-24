#include "NearAlpha.hlsli"

PSInput VSMain(float4 pos : POSITION)
{
	PSInput output = (PSInput)0;
	output.pos = mul(pos, world);
	output.sv_pos = mul(pos, wvp);
	return output;
}
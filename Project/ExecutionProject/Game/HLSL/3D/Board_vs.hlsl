#include "Board.hlsli"

PSInput VSMain( VSInput input )
{
	PSInput output = (PSInput)0;
	output.pos = mul(input.pos, wvp);
	output.normal = input.normal;
	output.uv = input.uv;
	return output;
}
#include "UnNormal.hlsli"

cbuffer Matrix : register(b0)
{
	row_major matrix wvp;
};

PSInput VSMain( VSInput input )
{
	PSInput output = (PSInput)0;
	output.pos = mul(input.pos, wvp);
	output.uv = input.uv;
	return output;
}
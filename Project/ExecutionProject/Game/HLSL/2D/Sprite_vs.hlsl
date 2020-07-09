#include "Sprite.hlsli"

PSInput VSMain(float4 pos : POSITION, float2 uv : TEXCOORD)
{
	PSInput output = (PSInput)0;
	output.svpos = pos;
	output.uv = uv;
	return output;
}
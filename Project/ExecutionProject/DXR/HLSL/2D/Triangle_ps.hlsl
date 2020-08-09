#include "Triangle.hlsli"

float4 PSMain(PSInput input) : SV_TARGET
{
	return input.color;
}
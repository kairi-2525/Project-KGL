#include "StaticObject.hlsli"

float4 main(PSInput input) : SV_TARGET
{
	return input.color;
}
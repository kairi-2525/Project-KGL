#include "Triangle.hlsli"

PSInput VSMain( VSInput input )
{
	PSInput output = (PSInput)0;

	output.position = input.position;
	output.color = input.color;

	return output;
}
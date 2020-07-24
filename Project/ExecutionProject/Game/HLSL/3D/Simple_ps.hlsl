cbuffer VSBuffer : register(b0)
{
	row_major float4x4 wvp;
};

float4 PSMain() : SV_TARGET
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
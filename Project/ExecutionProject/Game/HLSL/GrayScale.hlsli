// îíçïÇ…Ç∑ÇÈ
float4 GrayScale4(float4 col)
{
	const float3 Convert_YUV = { 0.299f, 0.578f, 0.114f };
	float Y = dot(col.rgb, Convert_YUV);
	return float4(Y, Y, Y, col.a);
}

float GrayScale(float4 col)
{
	const float3 Convert_YUV = { 0.299f, 0.578f, 0.114f };
	return dot(col.rgb, Convert_YUV);
}
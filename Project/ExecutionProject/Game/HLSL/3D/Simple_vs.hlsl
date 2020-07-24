cbuffer VSBuffer : register(b0)
{
	row_major float4x4 wvp;
};

float4 VSMain( float4 pos : POSITION ) : SV_POSITION
{
	return mul(pos, wvp);
}
struct PTC
{
	float4 a;
	float4 b;
	float4 c;
	float3 d;
	float e;
};

RWStructuredBuffer<PTC> nums : register(u0);

[numthreads(64, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	if (DTid.x >= 1) return;
	nums[DTid.x].a.x += 1.f;
	nums[DTid.x].b.x -= 1.f;
}
#include "Common.hlsl"

struct STriVertex
{
	float3 vertex;
	float4 color;
};

StructuredBuffer<STriVertex> BTriVertex : register(t0);

[shader("closesthit")] 
void ClosestHit(inout HitInfo payload, Attributes attrib) 
{
	float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

	uint vert_id = 3 * PrimitiveIndex();

	float3 hit_color = 
		BTriVertex[vert_id + 0].color * barycentrics.x +
		BTriVertex[vert_id + 1].color * barycentrics.y +
		BTriVertex[vert_id + 2].color * barycentrics.z;

	payload.colorAndDistance = float4(hit_color, RayTCurrent());
}

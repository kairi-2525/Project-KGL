#include "../Common.hlsl"

struct Vertex
{
	float3 position;
	float2 uv;
	float3 normal;
	float3 tangent;
	float3 bitangent;
};

StructuredBuffer<Vertex> vertices : register(t0);

[shader("closesthit")] 
void ClosestHit(inout HitInfo payload, Attributes attrib) 
{
	float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

	uint vert_id = 3 * PrimitiveIndex();

	float3 hit_color = 
		barycentrics.x +
		barycentrics.y +
		barycentrics.z;

	payload.colorAndDistance = float4(hit_color, RayTCurrent());
}

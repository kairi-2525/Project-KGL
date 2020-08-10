#include "Common.hlsl"

// Raytracing output texture, accessed as a UAV
// UAVとしてアクセスされるレイトレーシング出力テクスチャ
RWTexture2D< float4 > gOutput : register(u0);

// Raytracing acceleration structure, accessed as a SRV
// SRVとしてアクセスされるレイトレーシング加速構造
RaytracingAccelerationStructure SceneBVH : register(t0);

[shader("raygeneration")] 
void RayGen() {
	// Initialize the ray payload
	// 光線ペイロードを初期化する
	HitInfo payload;
	payload.colorAndDistance = float4(0.9, 0.6, 0.2, 1);

	// Get the location within the dispatched 2D grid of work items
	// (often maps to pixels, so this could represent a pixel coordinate).
	// 作業項目のディスパッチされた2Dグリッド内の場所を取得します
	//（多くの場合ピクセルにマップされるため、これはピクセル座標を表すことができます）。
	uint2 launchIndex = DispatchRaysIndex();

	gOutput[launchIndex] = float4(payload.colorAndDistance.rgb, 1.f);
}

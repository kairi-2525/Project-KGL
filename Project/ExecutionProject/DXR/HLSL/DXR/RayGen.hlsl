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
	payload.colorAndDistance = float4(0, 0, 0, 0);

	// Get the location within the dispatched 2D grid of work items
	// (often maps to pixels, so this could represent a pixel coordinate).
	// 作業項目のディスパッチされた2Dグリッド内の場所を取得します
	//（多くの場合ピクセルにマップされるため、これはピクセル座標を表すことができます）。
	uint2 launch_index = DispatchRaysIndex().xy;
	float2 dims = float2(DispatchRaysDimensions().xy);
	float2 d = (((launch_index.xy + 0.5f) / dims.xy) * 2.f - 1.f);

	// 原点、方向、最小-最大距離の値で構成される光線を定義します
	RayDesc ray;
	ray.Origin = float3(d.x, -d.y, 1);
	ray.Direction = float3(0, 0, -1);
	ray.TMin = 0;
	ray.TMax = 100000;

	// 光線を追跡する
	TraceRay(
		// パラメーター名：AccelerationStructure
		// 加速構造
		SceneBVH,

		// パラメータ名：RayFlags
		// フラグを使用して、サーフェスに当たったときの動作を指定できます
		RAY_FLAG_NONE,

		// パラメーター名：InstanceInclusionMask
		// インスタンスインクルージョンマスク。これを使用して、
		// ジオメトリマスクでマスクをAND-INGすることにより、
		// この光線に対して一部のジオメトリをマスクできます。 
		// 0xFFフラグは、ジオメトリがマスクされないことを示します
		0xFF,

		// パラメーター名：RayContributionToHitGroupIndex
		// レイのタイプに応じて、特定のオブジェクトに複数のヒットグループをアタッチすることができます
		// （つまり、通常のシェーディングを計算するために押すときの処理と、シャドウを計算するために押すときの処理）。 
		// これらのヒットグループはSBTで順番に指定されるため、
		// 以下の値は、このレイのヒットグループに適用するオフセット（4ビット）を示します。 
		// このサンプルでは、オブジェクトごとに1つのヒットグループしかないため、オフセットは0です。
		0,

		// パラメータ名：MultiplierForGeometryContributionToHitGroupIndex
		// SBTのオフセットは、オブジェクトID、そのインスタンスIDから計算できますが、
		// オブジェクトがアクセラレーション構造にプッシュされた順序によっても計算できます。 
		// これにより、アプリケーションは、ASに追加されたのと同じ順序でSBTのシェーダーをグループ化できます。
		// この場合、以下の値は、2つの連続するオブジェクト間のストライド（ヒットグループの数を表す4ビット）を表します。
		0,

		// パラメーター名：MissShaderIndex
		// SBTに複数の連続するミスシェーダーが存在する場合に使用するミスシェーダーのインデックス。 
		// これにより、ジオメトリにヒットしなかった場合のプログラムの動作を変更できます。
		// たとえば、通常のレンダリングでは空の色を返し、もう1つではシャドウレイの完全な可視性の値を返します。
		// このサンプルにはミスシェーダーが1つしかないため、インデックス0
		0,

		// パラメータ名：Ray
		// 追跡する光線情報
		ray,

		// パラメータ名：Payload
		// レイに関連付けられたペイロード。ヒット/ミスシェーダーとレイゲン間の通信に使用されます。
		payload
	);

	gOutput[launch_index] = float4(payload.colorAndDistance.rgb, 1.f);
}

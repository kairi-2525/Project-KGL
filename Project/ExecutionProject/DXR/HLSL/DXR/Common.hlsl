// Hit information, aka ray payload
// This sample only carries a shading color and hit distance.
// Note that the payload should be kept as small as possible,
// and that its size must be declared in the corresponding
// D3D12_RAYTRACING_SHADER_CONFIG pipeline subobjet.

// ヒット情報、別名rayペイロード
// このサンプルには、シェーディングカラーとヒット距離のみが含まれています。
// ペイロードはできるだけ小さく保つ必要があり、
// そのサイズは対応するD3D12_RAYTRACING_SHADER_CONFIGパイプラインサブオブジェクトで
// 宣言する必要があることに注意してください。
struct HitInfo
{
  float4 colorAndDistance;
};

// Attributes output by the raytracing when hitting a surface,
// here the barycentric coordinates
// サーフェスに当たったときにレイトレーシングによって出力される属性、ここでは重心座標
struct Attributes
{
  float2 bary;
};

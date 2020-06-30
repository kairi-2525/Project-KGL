#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <filesystem>
#include <DirectXMath.h>

namespace KGL
{
	inline namespace BASE
	{
		namespace PMD
		{
			struct Header
			{
				FLOAT version;
				CHAR model_name[20];
				CHAR comment[256];
			};
			struct Vertex
			{
				DirectX::XMFLOAT3 pos;
				DirectX::XMFLOAT3 normal;
				DirectX::XMFLOAT2 uv;
				USHORT bone_no[2];		// ボーン番号
				UCHAR bone_weight;		// ボーン影響度
				UCHAR edge_flg;			// 輪郭線フラグ
			};

#pragma pack(1)		// ここから１バイトアライメントになる
			struct Material
			{
				DirectX::XMFLOAT3 diffuse;	// ディフューズ色
				FLOAT alpha;				// ディフューズa
				FLOAT specularity;			// スペキュラの強さ
				DirectX::XMFLOAT3 specular;	// スペキュラ色
				DirectX::XMFLOAT3 ambient;	// アンビエント色
				UCHAR toon_idx;				// トゥーン番号
				UCHAR edge_flg;				// マテリアルごとの輪郭線フラグ
				// ここに２バイトのパディング #pragma pack(1)をすることで発生しない
				UINT indices_num;			// マテリアルが割り当てられるインデックス数
				CHAR tex_file_Path[20];		// テクスチャファイルパス+α
			};
#pragma pack()

			struct Desc
			{
				std::filesystem::path path;
				Header header;
				std::vector<UCHAR> vertices;
				std::vector<USHORT> indices;
				std::vector<Material> materials;
			};

			static constexpr size_t MT_SIZE = sizeof(Material);
			static constexpr size_t VERTEX_SIZE = 38u;
		}
	}
}
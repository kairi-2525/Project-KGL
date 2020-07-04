#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <filesystem>
#include <map>
#include <DirectXMath.h>

namespace KGL
{
	inline namespace BASE
	{
		namespace PMD
		{
			struct Header
			{
				FLOAT	version;
				CHAR	model_name[20];
				CHAR	comment[256];
			};
			struct Vertex
			{
				DirectX::XMFLOAT3	pos;
				DirectX::XMFLOAT3	normal;
				DirectX::XMFLOAT2	uv;
				USHORT				bone_no[2];		// ボーン番号
				UCHAR				bone_weight;	// ボーン影響度
				UCHAR				edge_flg;		// 輪郭線フラグ
			};

#pragma pack(1)		// ここから１バイトアライメントになる
			struct Material
			{
				DirectX::XMFLOAT3	diffuse;			// ディフューズ色
				FLOAT				alpha;				// ディフューズa
				FLOAT				specularity;		// スペキュラの強さ
				DirectX::XMFLOAT3	specular;			// スペキュラ色
				DirectX::XMFLOAT3	ambient;			// アンビエント色
				UCHAR				toon_idx;			// トゥーン番号
				UCHAR				edge_flg;			// マテリアルごとの輪郭線フラグ
				// ここに２バイトのパディング #pragma pack(1)をすることで発生しない
				UINT				indices_num;		// マテリアルが割り当てられるインデックス数
				CHAR				tex_file_Path[20];	// テクスチャファイルパス+α
			};
			struct Bone
			{
				CHAR				bone_name[20];	// ボーン名
				USHORT				parent_no;		// 親ボーン番号
				USHORT				next_no;		// 先端のボーン番号
				UCHAR				type;			// ボーンの種類
				// ここにアライメントが発生するため #pragma pack(1)
				USHORT				ik_bone_no;		// IKボーン番号
				DirectX::XMFLOAT3	pos;			// ボーンの基準点座標
			};
#pragma pack()
			struct BoneNode
			{
				INT						bone_idx;	// ボーンインデックス
				DirectX::XMFLOAT3		start_pos;	// ボーン基準点（回転の中心）
				DirectX::XMFLOAT3		end_pos;	// ボーン先端点（実際のスキニングには利用しない）
				std::vector<BoneNode*>	children;	// 子ノード
			};

			struct Desc
			{
				std::filesystem::path	path;
				Header					header;
				std::vector<UCHAR>		vertices;
				std::vector<USHORT>		indices;
				std::vector<Material>	materials;
				std::vector<Bone>		bones;
				std::map<std::string, BoneNode> bone_node_table;
			};

			static constexpr size_t MT_SIZE = sizeof(Material);
			static constexpr size_t VERTEX_SIZE = 38u;
		}
	}
}
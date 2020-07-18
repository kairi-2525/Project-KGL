#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <filesystem>
#include <map>
#include <DirectXMath.h>
#include <memory>
#include <unordered_map>

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

#pragma pack(1)		// ここから１バイトアライメントになる
			struct Vertex
			{
				DirectX::XMFLOAT3	pos;
				DirectX::XMFLOAT3	normal;
				DirectX::XMFLOAT2	uv;
				USHORT				bone_no[2];		// ボーン番号
				UCHAR				bone_weight;	// ボーン影響度
				UCHAR				edge_flg;		// 輪郭線フラグ
			};
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
				UINT32					bone_idx;		// ボーンインデックス
				UINT32					bone_type;		// ボーン種別
				UINT32					ik_parent_bone;	// IK親ボーン
				DirectX::XMFLOAT3		start_pos;		// ボーン基準点（回転の中心）
				DirectX::XMFLOAT3		end_pos;		// ボーン先端点（実際のスキニングには利用しない）
				std::vector<BoneNode*>	children;		// 子ノード
			};

			using BoneTable = std::map<std::string, BoneNode>;

			struct IK
			{
				UINT16					bone_idx;		// IK対象のボーンを示す
				UINT16					target_idx;		// ターゲットに近づけるためのボーンインデックス
				UINT16					iterations;		// 試行回数
				FLOAT					limit;			// 1回あたりの回転制限
				std::vector<UINT16>		node_idxes;		// 間のノード番号
			};

#pragma pack(1)
			struct Morph			// モーフ
			{
				CHAR						name[20];
				UINT32						vertex_count;
				UINT8						type;		
			};
#pragma pack()
			struct MorphVertex
			{
				UINT32						index;			// 表情用の頂点番号
				DirectX::XMFLOAT3			position;
			};
			struct MorpthData
			{
				std::string					name;
				UINT8						type;
				std::vector<MorphVertex>	vertices;
			};
			enum MORPH_TYPE
			{
				PMD_MORPH_TYPE_BASE,
				PMD_MORPH_TYPE_EYEBROW,
				PMD_MORPH_TYPE_EYE,
				PMD_MORPH_TYPE_LIP,
				PMD_MORPH_TYPE_OTHER
			};
#pragma pack(1)
			struct BoneLabelIndex
			{
				UINT16						bone_index;
				UINT8						frame_index;
			};
			struct LocalizeHeader
			{
				UINT8						flag;		// 1 : 英語対応あり
				CHAR						model_name[20];		// 英語名モデル
				CHAR						comment[256];
			};
			struct ToonTextureList
			{
				CHAR						file_name[10][100];
			};
#pragma pack()
			struct EnglishDesc
			{
				std::vector<std::string>			bone_names;
				std::vector<std::string>			morph_names;
				std::vector<std::string>			bone_labels;
			};
			using ToonTextureTable = std::unordered_map<UCHAR, std::filesystem::path>;
			struct Desc
			{
				std::filesystem::path				path;
				Header								header;
				std::vector<Vertex>					vertices;
				std::vector<USHORT>					indices;
				std::vector<Material>				materials;
				std::vector<Bone>					bones;
				std::vector<IK>						ik_data;
				BoneTable							bone_node_table;
				std::vector<std::string>			bone_name_array;
				std::vector<BoneNode*>				bone_node_address_array;
				std::vector<UINT32>					knee_idxes;
				std::vector<MorpthData>				morphs;
				std::vector<UINT16>					morph_label_indices;
				std::vector<std::string>			bone_labels;
				std::vector<BoneLabelIndex>			bone_label_indices;
				LocalizeHeader						localize_header;
				EnglishDesc							en;
				ToonTextureTable					toon_tex_table;
			};
			/*class AlignedVertex
			{
			public:
				Vertex vertex;
			public:
				const AlignedVertex& operator=(const Vertex& vert) noexcept
				{
					vertex.pos = vert.pos;
					vertex.uv = vert.uv;
					vertex.normal = vert.normal;
					vertex.edge_flg = vert.edge_flg;
					vertex.bone_no[0] = vert.bone_no[0];
					vertex.bone_no[1] = vert.bone_no[1];
					vertex.bone_weight = vert.bone_weight;
					return *this;
				}
				void* operator new(size_t size)
				{
					return _aligned_malloc(size, 1u);
				}
				void operator delete(void* p)
				{
					return _aligned_free(p);
				}
			};*/
			static constexpr size_t MT_SIZE = sizeof(Material);
			static constexpr size_t VERTEX_SIZE = sizeof(Vertex);
		}
	}
}
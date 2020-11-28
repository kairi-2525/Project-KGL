#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <filesystem>
#include <map>
#include <string>
#include <DirectXMath.h>
#include <memory>

namespace KGL
{
	inline namespace BASE
	{
		namespace OBJ
		{
			struct ObjectData
			{
				std::vector<DirectX::XMFLOAT3>	positions;
				std::vector<DirectX::XMFLOAT2>	uvs;
				std::vector<DirectX::XMFLOAT3>	normals;
			};

			struct Object
			{
				struct Desc
				{
					size_t begin;
					size_t count;
				};

				Desc positions;
				Desc uvs;
				Desc normals;
			};

			struct Vertex
			{
				UINT position;
				UINT uv;
				UINT normal;
			};

			using Vertices = std::vector<Vertex>;
			
			struct Material
			{
				struct Texture
				{
					struct TextureMap
					{
						float gain, contrast;
					};
					std::filesystem::path	path;
					bool					blend_u;			// 水平テクスチャ混合を設定 (標準は on)
					bool					blend_v;			// 垂直テクスチャ混合を設定 (標準は on)
					float					mipmap_boost;		// mip-mapのシャープさを押し上げ
					TextureMap				texture_map;		// テクスチャマップの値を変更 (標準は 0 1)
					DirectX::XMFLOAT3		offset_pos;			// 原点オフセット (標準は 0 0 0)
					DirectX::XMFLOAT3		offset_scale;		// スケールオフセット (標準は 1 1 1)
					DirectX::XMFLOAT3		offset_turbulence;	// 乱気流(?)オフセット (標準は 0 0 0)
					DirectX::XMUINT2		resolution;			// 作成するテクスチャ解像度
					bool					clamp;				// 0から1の範囲でクランプされたテクセルのみレンダリング (標準は off)
					float					bump_mult;			// バンプ乗数 (バンプマップ専用)
					std::string				bump_file_ch;		// スカラーまたはバンプテクスチャを作成するために
																// どのファイルのチャンネルを使用するか指定。
				};

				// パラメーター
				DirectX::XMFLOAT3									ambient_color;
				DirectX::XMFLOAT3									diffuse_color;
				DirectX::XMFLOAT3									specular_color;
				float												specular_weight;
				bool												specular_flg;
				float												dissolve;	// 透明度 1なら透明
				float												refraction;	// 屈折率
				bool												smooth;
				
				// テクスチャ
				std::shared_ptr<Texture>							tex_ambient;
				std::shared_ptr<Texture>							tex_diffuse;
				std::shared_ptr<Texture>							tex_specular;
				std::shared_ptr<Texture>							tex_specular_highlights;
				std::shared_ptr<Texture>							tex_dissolve;
				std::shared_ptr<Texture>							tex_bump;
				std::shared_ptr<Texture>							tex_displacement;
				std::shared_ptr<Texture>							tex_stencil_decal;

				std::map<std::string, std::string>					tex_reflections;

				// 頂点
				Vertices											vertices;
			};

			struct Desc
			{
				std::filesystem::path								mtl_path;
				std::map<std::string, std::shared_ptr<Material>>	materials;
				std::map<std::string, std::shared_ptr<Object>>		objects;
				ObjectData											object_data;
			};
		}
	}
}
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
		namespace FBX
		{
			const UINT MAX_BONE_INFLUENCES = 4u;
			const DirectX::XMFLOAT4X4 CORDINATE_CONVERSION =
			{
				+1, +0, +0, +0,
				+0, +1, +0, +0,
				+0, +0, -1, +0,
				+0, +0, +0, +1
			};
			struct Vertex
			{
				DirectX::XMFLOAT3	position;
				DirectX::XMFLOAT3	normal;
				DirectX::XMFLOAT2	uv;
				float				bone_weights[MAX_BONE_INFLUENCES] = { 1, 0, 0, 0 };
				int					bone_indices[MAX_BONE_INFLUENCES] = {};
			};
			struct Material
			{
				DirectX::XMFLOAT4		color = { 1.0f, 1.0f, 1.0f, 1.0f };
				std::filesystem::path	tex_file_pass;
			};
			struct Subset
			{
				UINT		index_start = 0;
				UINT		index_count = 0;
				Material	diffuse;
			};
			struct Bone_Influence
			{
				int		index; // index of bone
				float	weight; // weight of bone
			};
			using Bone_Influences_Per_Control_Point = std::vector<Bone_Influence>;
			struct Bone
			{
				DirectX::XMMATRIX transform;
			};
			using Skeletal = std::vector<Bone>;
			struct SkeletalAnimation : public std::vector<Skeletal>
			{
				float sampling_time = 1 / 24.0f;
				float animation_tick = 0.0f;
			};

			struct Mesh
			{
				std::vector<Subset>		subsets;
				DirectX::XMMATRIX		global_transform;
				std::vector<Vertex>		vertices;
				std::vector<UINT>		indexes;
				SkeletalAnimation		skeletal_animation;
			};

			struct Desc
			{
				std::vector<Mesh>		meshes;
				std::filesystem::path	path;
			};
		}
	}
}
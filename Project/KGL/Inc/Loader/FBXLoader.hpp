#pragma once

#include "../Base/FBX.hpp"
#include <memory>

#include <fbxsdk.h>
#pragma comment(lib, "libfbxsdk.lib")

namespace KGL
{
	inline namespace LOADER
	{
		class FBX_Loader
		{
		private:
			std::shared_ptr<const FBX::Desc> m_desc;
		private:
			static void FetchBoneMatrices(
				fbxsdk::FbxMesh* fbx_mesh,
				std::vector<FBX::Bone>& skeletal,
				fbxsdk::FbxTime time
			) noexcept;

			static void FetchAnimations(
				fbxsdk::FbxMesh* fbx_mesh,
				FBX::SkeletalAnimation& skeletal_animation,
				UINT sampling_rate = 0u
			) noexcept;

			static void FetchBoneInfluences(
				const FbxMesh* fbx_mesh,
				std::vector<FBX::Bone_Influences_Per_Control_Point>* influences
			) noexcept;

			static void Traverse(
				std::vector<fbxsdk::FbxNode*>* fetched_meshes,
				fbxsdk::FbxNode* node
			) noexcept;

			static void FbxMatrix_To_XMFLOAT4X4(
				const fbxsdk::FbxAMatrix& fbxamatrix,
				DirectX::XMFLOAT4X4* xmfloat4x4
			) noexcept;
		public:
			explicit FBX_Loader(std::filesystem::path path) noexcept;
			const std::shared_ptr<const FBX::Desc>& GetDesc() const noexcept { return m_desc; }
		};
	}
}
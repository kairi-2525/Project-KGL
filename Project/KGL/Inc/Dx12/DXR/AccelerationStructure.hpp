#pragma once

#include "../ConstantBuffer.hpp"
#include <vector>
#include <functional>
#include <DirectXMath.h>

namespace KGL
{
	inline namespace DX12
	{
		namespace DXR
		{
			struct ASResources
			{
				std::shared_ptr<KGL::Resource<CHAR>>	scratch;		// ASビルダーのスクラッチメモリ
				std::shared_ptr<KGL::Resource<CHAR>>	result;			// ASの場所
				std::shared_ptr<KGL::Resource<CHAR>>	instance_desc;	// インスタンスの行列を保持する
			};

			using BLAS = std::pair<std::shared_ptr<KGL::Resource<CHAR>>, DirectX::XMMATRIX>;

			struct BLASInstance
			{
				/// Bottom-level AS
				std::shared_ptr<KGL::Resource<CHAR>>	blas;
				/// Transform matrix
				DirectX::XMMATRIX						transform;
				/// Instance ID visible in the shader
				UINT									instance_id;
				/// Hit group index used to fetch the shaders from the SBT
				UINT									hit_group_index;
			};

			void ComputeASBufferSizes(
				ComPtrC<ID3D12Device5> device,
				bool allow_update,
				const std::vector<BLASInstance>& instances,
				UINT64* scratch_size_in_bytes,
				UINT64* result_size_in_bytes,
				UINT64* descriptors_size_in_bytes = nullptr
			);

			// Top Level Acceleration Structure
			[[nodiscard]] ASResources CreateTLAS(
				ComPtrC<ID3D12Device5> device,
				ComPtrC<ID3D12GraphicsCommandList4> cmd_list,
				const std::vector<BLAS>& instances,
				UINT hit_group_index = 0u
			);
		}
	}
}
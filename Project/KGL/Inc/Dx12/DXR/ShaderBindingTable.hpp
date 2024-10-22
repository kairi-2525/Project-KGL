#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <vector>
#include <string>

#include "../ConstantBuffer.hpp"

namespace KGL
{
	inline namespace DX12
	{
		namespace DXR
		{
			class ShaderBindingTable
			{
			private:
				struct PrivateEntryData
				{
					std::wstring entry_point;
					std::vector<void*> input_data;
				};
				struct PrivateDesc
				{
					std::vector<PrivateEntryData> raygen_table;
					std::vector<PrivateEntryData> miss_table;
					std::vector<PrivateEntryData> hit_group_table;

					UINT64 raygen_entry_size;
					UINT64 miss_entry_size;
					UINT64 hit_group_entry_size;
				};
			public:
				struct EntryData
				{
					std::string entry_point;
					std::vector<void*> input_data;
				public:
					EntryData() = default;
					explicit EntryData(
						const std::string& entry_point,
						const std::vector<void*>& input_data = {}
					) : entry_point(entry_point), input_data(input_data) {}
				};
				struct Desc
				{
					std::vector<EntryData> raygen_table;
					std::vector<EntryData> miss_table;
					std::vector<EntryData> hit_group_table;
				};
			private:
				PrivateDesc m_desc;
				std::shared_ptr<KGL::ResourcesBase> m_storage_resource;
			private:
				void ConvertDesc(const Desc& desc) noexcept;
			public:
				explicit ShaderBindingTable(
					ComPtrC<ID3D12Device> device,
					const Desc& desc
				) noexcept;
				D3D12_DISPATCH_RAYS_DESC GenerateRayDesc(
					D3D12_GPU_VIRTUAL_ADDRESS start_addres
				) const noexcept {
					D3D12_DISPATCH_RAYS_DESC ray_desc = {};
					GenerateRayDesc(&ray_desc, start_addres);
					return ray_desc;
				}
				D3D12_DISPATCH_RAYS_DESC GenerateRayDesc()
				{
					return GenerateRayDesc(m_storage_resource->Data()->GetGPUVirtualAddress());
				}
				void GenerateRayDesc(
					D3D12_DISPATCH_RAYS_DESC* out_desc,
					D3D12_GPU_VIRTUAL_ADDRESS start_addres
				) const noexcept;
				void GenerateRayDesc(
					D3D12_DISPATCH_RAYS_DESC* out_desc
				) const noexcept
				{
					return GenerateRayDesc(out_desc, m_storage_resource->Data()->GetGPUVirtualAddress());
				}
				UINT64 StorageSize() const noexcept;
			};
			using SBT = ShaderBindingTable;
		}
	}
}
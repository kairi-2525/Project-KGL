#pragma once

#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")

#include "../Helper/ComPtr.hpp"
#include "../Helper/ThrowAssert.hpp"

namespace KGL
{
	inline namespace DX12
	{
		class DescriptorHandle;
		class ResourcesBase
		{
		protected:
			ComPtr<ID3D12Resource> m_buffer;
			size_t m_size;
			UINT64 m_size_in_bytes;
		public:
			template<class _Ty>
			static UINT64 AlignmentSize(size_t size) noexcept { return ((sizeof(_Ty) * m_size) + 0xff) & ~0xff; };
		private:
			static HRESULT CreateCBV(std::shared_ptr<DescriptorHandle> p_handle, D3D12_GPU_VIRTUAL_ADDRESS address, ) const noexcept
		public:
			ResourcesBase(size_t size) noexcept : m_size(size) {}
			virtual ~ResourcesBase() = default;

			size_t Size() const noexcept { return m_size; }
			UINT64 SizeInBytes() const noexcept { return m_size_in_bytes; }
			template<class _Ty>
			HRESULT CreateCBV(std::shared_ptr<DescriptorHandle> p_handle) const noexcept
			{
				return CreateCBV<_Ty>(p_handle);
			}
			template<class _Ty>
			HRESULT CreateCBV(std::shared_ptr<DescriptorHandle> p_handle, UINT begin, UINT count = 1u) const noexcept
			{
				HRESULT hr = S_OK;
				if (p_handle)
				{
					const auto& desc = m_buffer->GetDesc();
					// CBV用Desc
					D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
					cbv_desc.BufferLocation = m_buffer->GetGPUVirtualAddress() + (begin * sizeof());
					cbv_desc.SizeInBytes = SCAST<UINT>(SizeInBytes());

					ComPtr<ID3D12Device> device;
					hr = m_buffer->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
					RCHECK_HR(hr, "m_buffer->GetDeviceに失敗");

					device->CreateConstantBufferView(&cbv_desc, p_handle->Cpu());
				}
				else
				{
					hr = E_FAIL;
				}
				return hr;
			}
		};

		template <class _Ty>
		class Resource : public ResourcesBase
		{
			_Ty* m_mapped_ptr;
		public:
			Resource(ComPtrC<ID3D12Device> device, size_t size,
				const D3D12_HEAP_PROPERTIES* prop = nullptr,
				D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE) noexcept :
				ResourcesBase(size), m_mapped_ptr(nullptr)
			{
				RCHECK(size == 0u, "無効なサイズ");
				m_size_in_bytes = AlignmentSize(m_size);
				D3D12_RESOURCE_DESC res_desc = {};
				res_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
				res_desc.Alignment = 0u;
				res_desc.Width = m_size_in_bytes;
				res_desc.Height = 1u;
				res_desc.DepthOrArraySize = 1u;
				res_desc.MipLevels = 1u;
				res_desc.Format = DXGI_FORMAT_UNKNOWN;
				res_desc.SampleDesc.Count = 1;
				res_desc.SampleDesc.Quality = 0;
				res_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
				res_desc.Flags = flag;
				if (prop)
				{
					auto hr = device->CreateCommittedResource(
						prop,
						D3D12_HEAP_FLAG_NONE,
						&res_desc,
						D3D12_RESOURCE_STATE_GENERIC_READ,
						nullptr,
						IID_PPV_ARGS(m_buffer.ReleaseAndGetAddressOf())
					);
					RCHECK(FAILED(hr), "CreateCommittedResourceに失敗");
				}
				else
				{
					D3D12_HEAP_PROPERTIES propeties = {};
					propeties.Type = D3D12_HEAP_TYPE_UPLOAD;
					propeties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
					propeties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
					propeties.CreationNodeMask = 1;
					propeties.VisibleNodeMask = 1;
					auto hr = device->CreateCommittedResource(
						&propeties,
						D3D12_HEAP_FLAG_NONE,
						&res_desc,
						D3D12_RESOURCE_STATE_GENERIC_READ,
						nullptr,
						IID_PPV_ARGS(m_buffer.ReleaseAndGetAddressOf())
					);
					RCHECK(FAILED(hr), "CreateCommittedResourceに失敗");
				}
			}
			_Ty* Map(UINT subresource = 0u, const D3D12_RANGE* p_read_range = nullptr)
			{
				_Ty* mapped_ptr = nullptr;
				auto hr = m_buffer->Map(subresource, p_read_range, (void**)&mapped_ptr);
				RCHECK(FAILED(hr), "無効なサイズ", nullptr);
				return mapped_ptr;
			}
			void Unmap(UINT subresource = 0u, const D3D12_RANGE* p_wwriten_range = nullptr)
			{
				m_buffer->Unmap(subresource, p_wwriten_range);
			}
			ComPtrC<ID3D12Resource> Data() const noexcept { return m_buffer; }
			UINT64 AlignmentSize(size_t size) const noexcept { return AlignmentSize<_Ty>(size); }
		};
	}
}
#pragma once

#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")

#include "../Helper/ComPtr.hpp"
#include "../Helper/ThrowAssert.hpp"
#include "../Helper/Cast.hpp"

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
			static UINT64 AlignmentStructSize(size_t size) noexcept { return AlignmentStructSize(size, sizeof(_Ty)); };
			static UINT64 AlignmentStructSize(size_t size, size_t struct_size) noexcept { return ((struct_size * size) + 0xff) & ~0xff; };
			static UINT64 AlignmentSize(size_t size) noexcept { return (size + 0xff) & ~0xff; };
		protected:
			HRESULT CreateCBV(
				D3D12_GPU_VIRTUAL_ADDRESS address,
				UINT size,
				std::shared_ptr<DescriptorHandle> p_handle
			) const noexcept;
		public:
			ResourcesBase(
				ComPtrC<ID3D12Device> device,
				size_t size, size_t struct_size,
				const D3D12_HEAP_PROPERTIES* prop,
				D3D12_RESOURCE_FLAGS flag
			) noexcept;
			virtual ~ResourcesBase() = default;

			ComPtrC<ID3D12Resource> Data() const noexcept { return m_buffer; }
			size_t Size() const noexcept { return m_size; }
			UINT64 SizeInBytes() const noexcept { return m_size_in_bytes; }
			HRESULT CreateCBV(std::shared_ptr<DescriptorHandle> p_handle) const noexcept
			{
				return CreateCBV(m_buffer->GetGPUVirtualAddress(), SCAST<UINT>(m_size_in_bytes), p_handle);
			}
		};

		template <class _Ty>
		class ResourceTyBase : public ResourcesBase
		{
		private:
			_Ty* m_mapped_ptr;
		protected:
			ResourceTyBase(
				ComPtrC<ID3D12Device> device,
				size_t size, size_t struct_size,
				const D3D12_HEAP_PROPERTIES* prop,
				D3D12_RESOURCE_FLAGS flag
			) noexcept :
				ResourcesBase(device, size, struct_size, prop, flag), m_mapped_ptr(nullptr)
			{
			}
		public:
			_Ty* Map(UINT subresource = 0u, const D3D12_RANGE& read_range = { 0, 0 })
			{
				_Ty* mapped_ptr = nullptr;
				auto hr = m_buffer->Map(subresource, &read_range, (void**)&mapped_ptr);
				RCHECK(FAILED(hr), "無効なサイズ", nullptr);
				return mapped_ptr;
			}
			void Unmap(UINT subresource = 0u, const D3D12_RANGE& wwriten_range = { 0, 0 })
			{
				m_buffer->Unmap(subresource, &wwriten_range);
			}
			UINT64 AlignmentStructSize(size_t size) const noexcept { return AlignmentStructSize<_Ty>(size); }
		};

		template <class _Ty>
		class Resource : public ResourceTyBase<_Ty>
		{
		public:
			Resource(ComPtrC<ID3D12Device> device, size_t size,
				const D3D12_HEAP_PROPERTIES* prop = nullptr,
				D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE
			) noexcept :
				ResourceTyBase<_Ty>(device, size, sizeof(_Ty), prop, flag)
			{
			}

			HRESULT CreateCBV(std::shared_ptr<DescriptorHandle> p_handle) const noexcept
			{
				return ResourcesBase::CreateCBV(ResourcesBase::m_buffer->GetGPUVirtualAddress(), SCAST<UINT>(ResourcesBase::m_size_in_bytes), p_handle);
			}
		};

		template <class _Ty>
		class MultiResource : public ResourceTyBase<_Ty>
		{
		public:
			MultiResource(ComPtrC<ID3D12Device> device, size_t size,
				const D3D12_HEAP_PROPERTIES* prop = nullptr,
				D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE
			) noexcept :
				ResourceTyBase<_Ty>(device, size, ResourcesBase::AlignmentStructSize<_Ty>(1u), prop, flag)
			{
			}

			// CBV を作成
			// @param begin 先頭
			// @param count beginから数えての要素数
			HRESULT CreateCBV(std::shared_ptr<DescriptorHandle> p_handle, UINT begin, UINT count = 1u) const noexcept
			{
				RCHECK(count <= 0, "count が小さすぎます", E_FAIL);
				RCHECK(begin + count > ResourcesBase::m_size, "begin + count が多きすぎます", E_FAIL);
				auto begin_address = ResourcesBase::m_buffer->GetGPUVirtualAddress();
				begin_address += begin * ResourcesBase::AlignmentStructSize<_Ty>(1u);
				return ResourcesBase::CreateCBV(begin_address, (sizeof(_Ty) * count), p_handle);
			}
			HRESULT CreateCBV(std::shared_ptr<DescriptorHandle> p_handle) const noexcept
			{
				return ResourcesBase::CreateCBV(ResourcesBase::m_buffer->GetGPUVirtualAddress(), SCAST<UINT>(ResourcesBase::m_size_in_bytes), p_handle);
			}
		};
	}
}
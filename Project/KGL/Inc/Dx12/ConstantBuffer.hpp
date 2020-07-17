#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")

#include "../Helper/ComPtr.hpp"
#include "../Helper/ThrowAssert.hpp"

namespace KGL
{
	inline namespace DX12
	{
		class ResourcesBase
		{
		protected:
			ComPtr<ID3D12Resource> m_buffer;
			size_t m_size;
		public:
			ResourcesBase(size_t size) noexcept : m_size(size) {}
			virtual ~ResourcesBase() = default;
			
			size_t Size() const noexcept { return m_size; };
		};

		template <class _Ty>
		class Resource : public ResourcesBase
		{
			_Ty* m_mapped_ptr;
		public:
			Resource(ComPtrC<ID3D12Device> device, size_t size) noexcept :
				ResourcesBase(size)
			{
				RCHECK(size == 0u, "無効なサイズ");
				const auto buffer_size = ((sizeof(_Ty) * m_size) + 0xff) & ~0xff;

				auto hr = device->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
					D3D12_HEAP_FLAG_NONE,
					&CD3DX12_RESOURCE_DESC::Buffer(buffer_size),
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(m_buffer.ReleaseAndGetAddressOf())
				);
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
		};
	}
}
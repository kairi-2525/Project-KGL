
#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")

#include "../Helper/ComPtr.hpp"
#include "../Helper/ThrowAssert.hpp"

namespace KGL
{
	inline namespace DX12
	{
		namespace HELPER
		{
			inline HRESULT CreateCommandAllocatorAndList(
				ComPtrC<ID3D12Device> device,
				ComPtr<ID3D12CommandAllocator>* p_allocator,
				ComPtr<ID3D12GraphicsCommandList>* p_list
			) {
				HRESULT hr = S_OK;

				if (p_allocator && !*p_allocator)
				{
					auto hr = device->CreateCommandAllocator(
						D3D12_COMMAND_LIST_TYPE_DIRECT,
						IID_PPV_ARGS(p_allocator->GetAddressOf())
					);
					RCHECK(FAILED(hr), "コマンドアロケーターの作成に失敗", hr);
				}
				if (p_allocator && *p_allocator && p_list)
				{
					hr = device->CreateCommandList(0,
						D3D12_COMMAND_LIST_TYPE_DIRECT,
						p_allocator->Get(), nullptr,
						IID_PPV_ARGS(p_list->ReleaseAndGetAddressOf())
					);
					RCHECK(FAILED(hr), "コマンドリストの作成に失敗", hr);
				}
				return hr;
			}
		}
	}
}
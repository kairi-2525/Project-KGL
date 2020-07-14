#pragma once

#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")

#include "../Helper/ComPtr.hpp"
#include "../Helper/Cast.hpp"
#include <string>

namespace KGL
{
	inline namespace DX12
	{
		inline namespace HELPER
		{
			//template<class _Ty = ComPtr<ID3D12Resource>>
			template <class _Ty = ID3D12Resource>
			inline HRESULT SetName(const ComPtr<_Ty>& data, INT64 num, std::wstring name0, std::wstring name1 = L"")
			{
				auto hr = data->SetName(
					(
						L"[" + std::to_wstring(num) + L"] "
						+ name0
						+ L"'s "
						+ name1
						+ L" Resource"
					).c_str()
				);
				assert(SUCCEEDED(hr) && "リソースの名付けに失敗");
				return hr;
			}
		}
	}
}
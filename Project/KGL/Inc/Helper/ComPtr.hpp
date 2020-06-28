#pragma once

#include <wrl.h>

namespace KGL
{
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	class UseComPtr
	{
	private:
		UseComPtr(const UseComPtr&) = delete;
		UseComPtr& operator=(const UseComPtr&) = delete;
	public:
		UseComPtr(LPVOID pvReserved = 0, DWORD dwCoInit = COINIT_MULTITHREADED) noexcept
		{ CoInitializeEx(pvReserved, dwCoInit); }
		~UseComPtr() noexcept { CoUninitialize(); }
	};
}
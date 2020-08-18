#pragma once

#include "DLL.hpp"
#include "../Helper/ComPtr.hpp"
#include <dxcapi.h>

namespace KGL
{
	inline namespace BASE
	{
		class DXC : public DLL
		{
		public:
			static inline const std::string DLLName = "dxcompiler.dll";
			static inline const std::string FuncNameCI = "DxcCreateInstance";
			static inline const std::string FuncNameCI2 = "DxcCreateInstance2";
		private:
			ComPtr<IDxcCompiler>		m_compiler;
			ComPtr<IDxcLibrary>			m_liblary;
			ComPtr<IDxcIncludeHandler>	m_dx_include_handler;
		public:
			explicit DXC(const std::filesystem::path& dll_path = DLLName) noexcept;

			ComPtrC<IDxcCompiler> GetCompiler() const noexcept { return m_compiler; }
			ComPtrC<IDxcLibrary> GetLiblary() const noexcept { return m_liblary; }
			ComPtrC<IDxcIncludeHandler> GetDXIHeader() const noexcept { return m_dx_include_handler; }

			template <class _TInterface>
			HRESULT CreateInstance(REFCLSID clsid, _Outptr_ _TInterface** pResult) noexcept
			{
				return CreateInstance(clsid, __uuidof(_TInterface), (IUnknown**)pResult);
			}

			HRESULT CreateInstance(REFCLSID clsid, REFIID riid, _Outptr_ IUnknown** pResult) noexcept
			{
				if (pResult == nullptr) return E_POINTER;
				if (m_dll == nullptr) return E_FAIL;
				HRESULT hr = ((DxcCreateInstanceProc)m_name_funcs.at(FuncNameCI))(clsid, riid, (LPVOID*)pResult);
				return hr;
			}

			template <typename _TInterface>
			HRESULT CreateInstance2(IMalloc* pMalloc, REFCLSID clsid, _Outptr_ _TInterface** pResult) {
				return CreateInstance2(pMalloc, clsid, __uuidof(_TInterface), (IUnknown**)pResult);
			}

			HRESULT CreateInstance2(IMalloc* pMalloc, REFCLSID clsid, REFIID riid, _Outptr_ IUnknown** pResult) {
				if (pResult == nullptr) return E_POINTER;
				if (m_dll == nullptr) return E_FAIL;
				const auto& func = m_name_funcs.find(FuncNameCI2);
				if (func == m_name_funcs.end()) return E_FAIL;
				HRESULT hr = ((DxcCreateInstance2Proc)func->second)(pMalloc, clsid, riid, (LPVOID*)pResult);
				return hr;
			}
		};
	}
}
#include <Base/DXC.hpp>
#include <Helper/ThrowAssert.hpp>

using namespace KGL;

DXC::DXC(const std::filesystem::path& dll_path) noexcept : DLL(dll_path)
{
	HRESULT hr = S_OK;
	hr = Load(FuncNameCI);
	RCHECK_STR(FAILED(hr), "ŠÖ” " + FuncNameCI + " ‚Ì“Ç‚Ýž‚Ý‚ÉŽ¸”s", "Load(FuncNameCI)‚ÉŽ¸”s");
	Load(FuncNameCI2); // Ž¸”s‚µ‚Ä‚àOK

	hr = CreateInstance(CLSID_DxcCompiler, m_compiler.GetAddressOf());
	RCHECK(FAILED(hr), "compiler ‚Ì DxcCreateInstance ‚ÉŽ¸”s");
	hr = CreateInstance(CLSID_DxcLibrary, m_liblary.GetAddressOf());
	RCHECK(FAILED(hr), "liblary ‚Ì DxcCreateInstance ‚ÉŽ¸”s");
	hr = m_liblary->CreateIncludeHandler(m_dx_include_handler.GetAddressOf());
	RCHECK(FAILED(hr), "dx_include_handler ‚Ì CreateIncludeHandler ‚ÉŽ¸”s");
}
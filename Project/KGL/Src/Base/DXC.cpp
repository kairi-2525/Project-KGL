#include <Base/DXC.hpp>
#include <Helper/ThrowAssert.hpp>

using namespace KGL;

DXC::DXC(const std::filesystem::path& dll_path) noexcept : DLL(dll_path)
{
	HRESULT hr = S_OK;
	hr = Load(FuncNameCI);
	RCHECK_STR(FAILED(hr), "�֐� " + FuncNameCI + " �̓ǂݍ��݂Ɏ��s", "Load(FuncNameCI)�Ɏ��s");
	Load(FuncNameCI2); // ���s���Ă�OK

	hr = CreateInstance(CLSID_DxcCompiler, m_compiler.GetAddressOf());
	RCHECK(FAILED(hr), "compiler �� DxcCreateInstance �Ɏ��s");
	hr = CreateInstance(CLSID_DxcLibrary, m_liblary.GetAddressOf());
	RCHECK(FAILED(hr), "liblary �� DxcCreateInstance �Ɏ��s");
	hr = m_liblary->CreateIncludeHandler(m_dx_include_handler.GetAddressOf());
	RCHECK(FAILED(hr), "dx_include_handler �� CreateIncludeHandler �Ɏ��s");
}
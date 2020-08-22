#include <Base/DLL.hpp>
#include <Helper/ThrowAssert.hpp>

using namespace KGL;

DLL::DLL(const std::filesystem::path& name) noexcept
{
//#ifdef _WIN32
	m_dll = LoadLibraryA(name.string().c_str());
//#else
//	char name_str[256];
//	std::wcstombs(name_str, name.wstring().c_str(), 256);
//	m_dll = ::dlopen(name_str, RTLD_LAZY);
//#endif
	try
	{
		if (m_dll == nullptr)
		{
			HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
			throw std::runtime_error(name.string() + " ‚Ì“Ç‚İ‚İ‚É¸”sB");
		}
	}
	catch (std::runtime_error& exception)
	{
		RuntimeErrorStop(exception);
	}
}

DLL::~DLL()
{
	if (m_dll) FreeLibrary(m_dll);
}

HRESULT DLL::Load(std::string func_name, int num) noexcept
{
	if (m_name_funcs.count(func_name) == 1u)
		return E_FAIL;

	auto& func = m_name_funcs[func_name];
	func = GetProcAddress(m_dll, func_name.c_str());

	HRESULT hr = S_OK;
	if (func == nullptr)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}

	if (num >= 0)
	{
		if (m_num_funcs.count(num) == 1u)
			return hr;

		m_num_funcs[num] = func;
	}

	return hr;
}
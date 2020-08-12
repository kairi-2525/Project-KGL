#include <Base/Library.hpp>
#include <Helper/ThrowAssert.hpp>

using namespace KGL;

Library::Library(const std::filesystem::path& name) noexcept
{
	m_lib = LoadLibraryA(name.string().c_str());
	try
	{
		if (m_lib == nullptr)
		{
			HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
			throw std::runtime_error(name.string() + " ÇÃì«Ç›çûÇ›Ç…é∏îsÅB");
		}
	}
	catch (std::runtime_error& exception)
	{
		RuntimeErrorStop(exception);
	}
}
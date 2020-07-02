#include <Dx12/Shader.hpp>
#include <Loader/Loader.hpp>
#include <Helper/ThrowAssert.hpp>

#pragma comment(lib, "d3dcompiler.lib")

using namespace KGL;

HRESULT Shader::Load(const Desc& desc,
	_In_reads_opt_(_Inexpressible_(pDefines->Name != NULL)) CONST D3D_SHADER_MACRO* p_defines,
	_In_opt_ ID3DInclude* p_include,
	_In_ UINT flag0, _In_ UINT flag1,
	ComPtr<ID3DBlob>* code,
	_Always_(_Outptr_opt_result_maybenull_) ID3DBlob** pp_error_msg
) noexcept(false)
{
	assert(!(*code) && "pp_codeÇ™nullptrÇ≈ÇÕÇ†ÇËÇ‹ÇπÇÒÅB");
	HRESULT hr;
	hr = D3DCompileFromFile(
		desc.hlsl.c_str(),
		p_defines,
		p_include,
		desc.entry_point.c_str(),
		desc.version.c_str(),
		flag0, flag1,
		code->ReleaseAndGetAddressOf(),
		pp_error_msg
	);
	if (!IsFound(hr))
	{
		throw std::runtime_error(
			"[ " + desc.hlsl.string() + " ] "
			+ "Ç™å©Ç¬Ç©ÇËÇ‹ÇπÇÒÇ≈ÇµÇΩÅB"
		);
	}
	return hr;
}

Shader::Shader(const Desc& vs, const Desc& ps,
	const std::vector<D3D12_INPUT_ELEMENT_DESC>& input_desc,
	_In_reads_opt_(_Inexpressible_(pDefines->Name != NULL)) CONST D3D_SHADER_MACRO* p_defines,
	_In_opt_ ID3DInclude* p_include,
	_In_ UINT flag0, _In_ UINT flag1
) noexcept
{
	ComPtr<ID3DBlob> error_code;
	HRESULT hr;
	try
	{
		// VS
		hr = Load(
			vs,
			p_defines,
			p_include,
			flag0, flag1,
			&m_vs,
			error_code.ReleaseAndGetAddressOf()
		);
		RCHECK(FAILED(hr), "VSÇÃê∂ê¨Ç…é∏îs");
		// PS
		hr = Load(
			ps,
			p_defines,
			p_include,
			flag0, flag1,
			&m_ps,
			error_code.ReleaseAndGetAddressOf()
		);
		RCHECK(FAILED(hr), "PSÇÃê∂ê¨Ç…é∏îs");
	}
	catch (std::runtime_error& exception)
	{
		RuntimeErrorStop(exception);
	}

	m_input_desc = input_desc;
}
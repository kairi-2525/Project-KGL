#include <Dx12/Shader.hpp>
#include <Loader/Loader.hpp>
#include <Helper/ThrowAssert.hpp>

#pragma comment(lib, "d3dcompiler.lib")

using namespace KGL;

HRESULT SHADER::Load(const Desc& desc,
	_In_reads_opt_(_Inexpressible_(pDefines->Name != NULL)) CONST D3D_SHADER_MACRO* p_defines,
	_In_opt_ ID3DInclude* p_include,
	_In_ UINT flag0, _In_ UINT flag1,
	ComPtr<ID3DBlob>* code,
	_Always_(_Outptr_opt_result_maybenull_) ID3DBlob** pp_error_msg
) noexcept(false)
{
	assert(!(*code) && "pp_code��nullptr�ł͂���܂���B");
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
			+ "��������܂���ł����B"
		);
	}
	return hr;
}

Shader::Shader(
	const SHADER::Desc& vs, const SHADER::Desc& ps,
	const SHADER::Desc& ds, const SHADER::Desc& hs, const SHADER::Desc& gs,
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
		if (!vs.hlsl.empty())
		{
			hr = Load(
				vs,
				p_defines,
				p_include,
				flag0, flag1,
				&m_vs,
				error_code.ReleaseAndGetAddressOf()
			);
			RCHECK(FAILED(hr), "VS�̐����Ɏ��s");
		}
		// PS
		if (!ps.hlsl.empty())
		{
			// PS
			hr = Load(
				ps,
				p_defines,
				p_include,
				flag0, flag1,
				&m_ps,
				error_code.ReleaseAndGetAddressOf()
			);
			RCHECK(FAILED(hr), "PS�̐����Ɏ��s");
		}
		// DS
		if (!ds.hlsl.empty())
		{
			hr = Load(
				ds,
				p_defines,
				p_include,
				flag0, flag1,
				&m_ds,
				error_code.ReleaseAndGetAddressOf()
			);
			RCHECK(FAILED(hr), "DS�̐����Ɏ��s");
		}
		// HS
		if (!hs.hlsl.empty())
		{
			// HS
			hr = Load(
				hs,
				p_defines,
				p_include,
				flag0, flag1,
				&m_hs,
				error_code.ReleaseAndGetAddressOf()
			);
			RCHECK(FAILED(hr), "HS�̐����Ɏ��s");
		}
		// GS
		if (!gs.hlsl.empty())
		{
			// GS
			hr = Load(
				gs,
				p_defines,
				p_include,
				flag0, flag1,
				&m_gs,
				error_code.ReleaseAndGetAddressOf()
			);
			RCHECK(FAILED(hr), "GS�̐����Ɏ��s");
		}
	}
	catch (std::runtime_error& exception)
	{
		RuntimeErrorStop(exception);
	}

	m_input_desc = input_desc;
}

ShaderCS::ShaderCS(const SHADER::Desc& cs,
	_In_reads_opt_(_Inexpressible_(pDefines->Name != NULL)) CONST D3D_SHADER_MACRO* p_defines,
	_In_opt_ ID3DInclude* p_include,
	_In_ UINT flag0, _In_ UINT flag1
) noexcept
{
	ComPtr<ID3DBlob> error_code;
	HRESULT hr;
	try
	{
		// CS
		if (!cs.hlsl.empty())
		{
			// CS
			hr = Load(
				cs,
				p_defines,
				p_include,
				flag0, flag1,
				&m_cs,
				error_code.ReleaseAndGetAddressOf()
			);
			RCHECK(FAILED(hr), "PS�̐����Ɏ��s");
		}
	}
	catch (std::runtime_error& exception)
	{
		RuntimeErrorStop(exception);
	}
}
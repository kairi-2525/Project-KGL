#include <Dx12/Shader.hpp>
#include <Loader/Loader.hpp>
#include <Helper/ThrowAssert.hpp>

//#pragma comment(lib, "dxcompiler")
#include <fstream>
#include <sstream>
#include <comdef.h>

//#pragma comment(lib, "d3dcompiler.lib")

using namespace KGL;

//HRESULT SHADER::Load(const Desc& desc,
//	_In_reads_opt_(_Inexpressible_(pDefines->Name != NULL)) CONST D3D_SHADER_MACRO* p_defines,
//	_In_opt_ ID3DInclude* p_include,
//	_In_ UINT flag0, _In_ UINT flag1,
//	ComPtr<ID3DBlob>* code,
//	_Always_(_Outptr_opt_result_maybenull_) ID3DBlob** pp_error_msg
//) noexcept(false)

HRESULT SHADER::Load(const std::shared_ptr<DXC> dxc, const Desc& desc, ComPtr<ID3DBlob>* code
) noexcept(false)
{
	HRESULT hr;

	RCHECK(!dxc, "dxc �� nullptr�B", E_FAIL);

	// �V�F�[�_�[�t�@�C����ǂݍ���
	std::ifstream shader_file(desc.hlsl.string());
	
	if (!shader_file.good())
	{
		throw std::runtime_error(
			"[ " + desc.hlsl.string() + " ] "
			+ "��������܂���ł����B"
		);
	}

	std::stringstream str_stream;
	str_stream << shader_file.rdbuf();
	std::string str_shader = str_stream.str();

	// string ���� shader blob ���쐬
	RCHECK((*code), "pp_code��nullptr�ł͂���܂���B", E_FAIL);
	KGL::ComPtr<IDxcBlobEncoding> text_blob;
	hr = dxc->GetLiblary()->CreateBlobWithEncodingFromPinned(
		(LPBYTE)str_shader.c_str(), SCAST<uint32_t>(str_shader.size()), 0,
		text_blob.GetAddressOf()
	);
	RCHECK_HR(hr, "CreateBlobWithEncodingFromPinned �Ɏ��s");

	// �R���p�C��
	KGL::ComPtr<IDxcOperationResult> result;
	const std::filesystem::path entry_point = desc.entry_point;
	std::filesystem::path version = desc.version;
	{
		auto pos = version.string().find("5_");
		if (pos != std::string::npos)
		{
			auto str = version.string();
			str.replace(pos, 3, "6_0");
			version = str;
		}
	}
	LPCWSTR compile_flags[] = {
#if _DEBUG
		L"Zi", L"/O0"
#else
		L"/02"	// �����[�X�r���h�ł͍œK��
#endif
	};
	hr = dxc->GetCompiler()->Compile(
		text_blob.Get(),
		desc.hlsl.wstring().c_str(),
		entry_point.wstring().c_str(),
		version.wstring().c_str(),
		compile_flags, SCAST<UINT32>(std::size(compile_flags))
		, nullptr, 0,
		dxc->GetDXIHeader().Get(),
		result.GetAddressOf()
		);
	RCHECK_HR(hr, "IDXC �� Compile �Ɏ��s");

	HRESULT result_stats;
	hr = result->GetStatus(&result_stats);
	RCHECK_HR(hr, "IDXC �� result->GetStatus �Ɏ��s");
	if (FAILED(result_stats))
	{
		KGL::ComPtr<IDxcBlobEncoding> error;
		hr = result->GetErrorBuffer(&error);
		if (FAILED(hr))
		{
			throw std::runtime_error("�V�F�[�_�[�R���p�C���G���[�̎擾�Ɏ��s���܂���");
		}

		// �G���[ BLOB �� string �ɕϊ�����
		std::vector<char> info_log(error->GetBufferSize() + 1);
		memcpy(info_log.data(), error->GetBufferPointer(), error->GetBufferSize());
		info_log[error->GetBufferSize()] = 0;

		std::string error_msg = "�V�F�[�_�[�R���p�C���G���[:\n";
		error_msg.append(info_log.data());

		MessageBoxA(nullptr, error_msg.c_str(), "�G���[�I", MB_OK);
		throw std::runtime_error("�V�F�[�_�[�R���p�C���G���[�I");
	}

	_COM_SMARTPTR_TYPEDEF(IDxcBlob, __uuidof(IDxcBlob));
	IDxcBlobPtr p_idxc_blob;
	_COM_SMARTPTR_TYPEDEF(ID3DBlob, __uuidof(ID3DBlob));
	ID3DBlobPtr p_id3d_blob;

	hr = result->GetResult(&p_idxc_blob);
	p_idxc_blob.AddRef();
	p_id3d_blob = p_idxc_blob;
	code->Attach(p_id3d_blob);
	//p_idxc_blob = p_id3d_blob = nullptr;

	RCHECK_HR(hr, "IDXC �� GetResult �Ɏ��s");

	
	/*hr = D3DCompileFromFile(
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
	return hr;*/
}

//Shader::Shader(
//	const SHADER::Desc& vs, const SHADER::Desc& ps,
//	const SHADER::Desc& ds, const SHADER::Desc& hs, const SHADER::Desc& gs,
//	const std::vector<D3D12_INPUT_ELEMENT_DESC>& input_desc,
//	_In_reads_opt_(_Inexpressible_(pDefines->Name != NULL)) CONST D3D_SHADER_MACRO* p_defines,
//	_In_opt_ ID3DInclude* p_include,
//	_In_ UINT flag0, _In_ UINT flag1
Shader::Shader(
	const std::shared_ptr<DXC> dxc,
	const SHADER::Desc& vs, const SHADER::Desc& ps,
	const SHADER::Desc& ds, const SHADER::Desc& hs, const SHADER::Desc& gs,
	const std::vector<D3D12_INPUT_ELEMENT_DESC>& input_desc
) noexcept
{
	ComPtr<ID3DBlob> error_code;
	HRESULT hr;
	try
	{
		// VS
		if (!vs.hlsl.empty())
		{
			hr = Load(dxc, vs, &m_vs);
			RCHECK(FAILED(hr), "VS�̐����Ɏ��s");
		}
		// PS
		if (!ps.hlsl.empty())
		{
			// PS
			hr = Load(dxc, ps, &m_ps);
			RCHECK(FAILED(hr), "PS�̐����Ɏ��s");
		}
		// DS
		if (!ds.hlsl.empty())
		{
			hr = Load(dxc, ds, &m_ds);
			RCHECK(FAILED(hr), "DS�̐����Ɏ��s");
		}
		// HS
		if (!hs.hlsl.empty())
		{
			// HS
			hr = Load(dxc, hs, &m_hs);
			RCHECK(FAILED(hr), "HS�̐����Ɏ��s");
		}
		// GS
		if (!gs.hlsl.empty())
		{
			// GS
			hr = Load(dxc, gs, &m_gs);
			RCHECK(FAILED(hr), "GS�̐����Ɏ��s");
		}
	}
	catch (std::runtime_error& exception)
	{
		RuntimeErrorStop(exception);
	}

	m_input_desc = input_desc;
}

//ShaderCS::ShaderCS(const SHADER::Desc& cs,
//	_In_reads_opt_(_Inexpressible_(pDefines->Name != NULL)) CONST D3D_SHADER_MACRO* p_defines,
//	_In_opt_ ID3DInclude* p_include,
//	_In_ UINT flag0, _In_ UINT flag1
ShaderCS::ShaderCS(const std::shared_ptr<DXC> dxc, const SHADER::Desc& cs) noexcept
{
	ComPtr<ID3DBlob> error_code;
	HRESULT hr;
	try
	{
		// CS
		if (!cs.hlsl.empty())
		{
			// CS
			hr = Load(dxc, cs, &m_cs);
			RCHECK(FAILED(hr), "CS�̐����Ɏ��s");
		}
	}
	catch (std::runtime_error& exception)
	{
		RuntimeErrorStop(exception);
	}
}
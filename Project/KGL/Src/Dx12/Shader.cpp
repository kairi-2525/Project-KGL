#include <Dx12/Shader.hpp>
#include <Loader/Loader.hpp>
#include <Helper/ThrowAssert.hpp>

#pragma comment(lib, "dxcompiler")
#include <fstream>
#include <sstream>

#pragma comment(lib, "d3dcompiler.lib")

using namespace KGL;

//HRESULT SHADER::Load(const Desc& desc,
//	_In_reads_opt_(_Inexpressible_(pDefines->Name != NULL)) CONST D3D_SHADER_MACRO* p_defines,
//	_In_opt_ ID3DInclude* p_include,
//	_In_ UINT flag0, _In_ UINT flag1,
//	ComPtr<ID3DBlob>* code,
//	_Always_(_Outptr_opt_result_maybenull_) ID3DBlob** pp_error_msg
//) noexcept(false)

HRESULT SHADER::Load(const Desc& desc, ComPtr<IDxcBlob>* code
) noexcept(false)
{
	KGL::ComPtr<IDxcCompiler>		compiler;
	KGL::ComPtr<IDxcLibrary>		liblary;
	KGL::ComPtr<IDxcIncludeHandler> dx_include_handler;

	HRESULT hr;

	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(compiler.GetAddressOf()));
	RCHECK_HR(hr, "compiler の DxcCreateInstance に失敗");
	hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(liblary.GetAddressOf()));
	RCHECK_HR(hr, "liblary の DxcCreateInstance に失敗");
	hr = liblary->CreateIncludeHandler(dx_include_handler.GetAddressOf());
	RCHECK_HR(hr, "dx_include_handler の CreateIncludeHandler に失敗");

	// シェーダーファイルを読み込み
	std::ifstream shader_file(desc.hlsl.string());
	
	if (!shader_file.good())
	{
		throw std::runtime_error(
			"[ " + desc.hlsl.string() + " ] "
			+ "が見つかりませんでした。"
		);
	}

	std::stringstream str_stream;
	str_stream << shader_file.rdbuf();
	std::string str_shader = str_stream.str();

	// string から shader blob を作成
	assert(!(*code) && "pp_codeがnullptrではありません。");
	KGL::ComPtr<IDxcBlobEncoding> text_blob;
	hr = liblary->CreateBlobWithEncodingFromPinned(
		(LPBYTE)str_shader.c_str(), SCAST<uint32_t>(str_shader.size()), 0,
		text_blob.GetAddressOf()
	);
	RCHECK_HR(hr, "CreateBlobWithEncodingFromPinned に失敗");

	// コンパイル
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
		L"/02"	// リリースビルドでは最適化
#endif
	};
	hr = compiler->Compile(
		text_blob.Get(),
		desc.hlsl.wstring().c_str(),
		entry_point.wstring().c_str(),
		version.wstring().c_str(),
		compile_flags, SCAST<UINT32>(std::size(compile_flags))
		, nullptr, 0,
		dx_include_handler.Get(),
		result.GetAddressOf()
		);
	RCHECK_HR(hr, "IDXC の Compile に失敗");

	HRESULT result_stats;
	hr = result->GetStatus(&result_stats);
	RCHECK_HR(hr, "IDXC の result->GetStatus に失敗");
	if (FAILED(result_stats))
	{
		KGL::ComPtr<IDxcBlobEncoding> error;
		hr = result->GetErrorBuffer(&error);
		if (FAILED(hr))
		{
			throw std::runtime_error("シェーダーコンパイラエラーの取得に失敗しました");
		}

		// エラー BLOB を string に変換する
		std::vector<char> info_log(error->GetBufferSize() + 1);
		memcpy(info_log.data(), error->GetBufferPointer(), error->GetBufferSize());
		info_log[error->GetBufferSize()] = 0;

		std::string error_msg = "シェーダーコンパイラエラー:\n";
		error_msg.append(info_log.data());

		MessageBoxA(nullptr, error_msg.c_str(), "エラー！", MB_OK);
		throw std::runtime_error("シェーダーコンパイルエラー！");
	}
	hr = result->GetResult(code->ReleaseAndGetAddressOf());
	RCHECK_HR(hr, "IDXC の GetResult に失敗");

	
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
			+ "が見つかりませんでした。"
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
			hr = Load(vs, &m_vs);
			RCHECK(FAILED(hr), "VSの生成に失敗");
		}
		// PS
		if (!ps.hlsl.empty())
		{
			// PS
			hr = Load(ps, &m_ps);
			RCHECK(FAILED(hr), "PSの生成に失敗");
		}
		// DS
		if (!ds.hlsl.empty())
		{
			hr = Load(ds, &m_ds);
			RCHECK(FAILED(hr), "DSの生成に失敗");
		}
		// HS
		if (!hs.hlsl.empty())
		{
			// HS
			hr = Load(hs, &m_hs);
			RCHECK(FAILED(hr), "HSの生成に失敗");
		}
		// GS
		if (!gs.hlsl.empty())
		{
			// GS
			hr = Load(gs, &m_gs);
			RCHECK(FAILED(hr), "GSの生成に失敗");
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
ShaderCS::ShaderCS(const SHADER::Desc& cs) noexcept
{
	ComPtr<ID3DBlob> error_code;
	HRESULT hr;
	try
	{
		// CS
		if (!cs.hlsl.empty())
		{
			// CS
			hr = Load(cs, &m_cs);
			RCHECK(FAILED(hr), "PSの生成に失敗");
		}
	}
	catch (std::runtime_error& exception)
	{
		RuntimeErrorStop(exception);
	}
}
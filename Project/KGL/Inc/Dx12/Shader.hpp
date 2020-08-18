#pragma once

#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")

#include "../Helper/ComPtr.hpp"
#include "../Helper/Cast.hpp"
#include "../Base/DXC.hpp"
#include <filesystem>
#include <string>
#include <vector>
#include <cassert>


namespace KGL
{
	inline namespace DX12
	{
		namespace SHADER
		{
			struct Desc
			{
				std::filesystem::path	hlsl;
				std::string				entry_point;
				std::string				version;
			};

			/*HRESULT Load(const SHADER::Desc& desc,
				_In_reads_opt_(_Inexpressible_(pDefines->Name != NULL)) CONST D3D_SHADER_MACRO* p_defines,
				_In_opt_ ID3DInclude* p_include,
				_In_ UINT flag0, _In_ UINT flag1,
				ComPtr<ID3DBlob>* code,
				_Always_(_Outptr_opt_result_maybenull_) ID3DBlob** pp_error_msg
			) noexcept(false);*/
			HRESULT Load(const std::shared_ptr<DXC> dxc, const SHADER::Desc& desc, ComPtr<ID3DBlob>* code) noexcept(false);
		}

		class Shader
		{
		private:
			ComPtr<ID3DBlob> m_vs, m_ps, m_ds, m_hs, m_gs;
			//ComPtr<IDxcBlob> m_vs, m_ps, m_ds, m_hs, m_gs;
			std::vector<D3D12_INPUT_ELEMENT_DESC> m_input_desc;
		public:
			const ComPtr<ID3DBlob>& GetVS() const noexcept { return m_vs; }
			const ComPtr<ID3DBlob>& GetPS() const noexcept { return m_ps; }
			const ComPtr<ID3DBlob>& GetDS() const noexcept { return m_ds; }
			const ComPtr<ID3DBlob>& GetHS() const noexcept { return m_hs; }
			const ComPtr<ID3DBlob>& GetGS() const noexcept { return m_gs; }

			/*const ComPtr<IDxcBlob>& GetVS() const noexcept { return m_vs; }
			const ComPtr<IDxcBlob>& GetPS() const noexcept { return m_ps; }
			const ComPtr<IDxcBlob>& GetDS() const noexcept { return m_ds; }
			const ComPtr<IDxcBlob>& GetHS() const noexcept { return m_hs; }
			const ComPtr<IDxcBlob>& GetGS() const noexcept { return m_gs; }*/

			void GetDesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc) const noexcept;
			
			/*Shader(
				const SHADER::Desc& vs,
				const std::vector<D3D12_INPUT_ELEMENT_DESC>& input_desc,
				_In_reads_opt_(_Inexpressible_(pDefines->Name != NULL)) CONST D3D_SHADER_MACRO* p_defines = nullptr,
				_In_opt_ ID3DInclude* p_include = D3D_COMPILE_STANDARD_FILE_INCLUDE,
				_In_ UINT flag0 = 0, _In_ UINT flag1 = 0
			) noexcept : Shader(vs, {}, {}, {}, {}, input_desc, p_defines, p_include, flag0, flag1) {}
			Shader(
				const SHADER::Desc& vs, const SHADER::Desc& ps,
				const std::vector<D3D12_INPUT_ELEMENT_DESC>& input_desc,
				_In_reads_opt_(_Inexpressible_(pDefines->Name != NULL)) CONST D3D_SHADER_MACRO* p_defines = nullptr,
				_In_opt_ ID3DInclude* p_include = D3D_COMPILE_STANDARD_FILE_INCLUDE,
				_In_ UINT flag0 = 0, _In_ UINT flag1 = 0
			) noexcept : Shader(vs, ps, {}, {}, {}, input_desc, p_defines, p_include, flag0, flag1) {}
			Shader(
				const SHADER::Desc& vs, const SHADER::Desc& ps, const SHADER::Desc& ds,
				const std::vector<D3D12_INPUT_ELEMENT_DESC>& input_desc,
				_In_reads_opt_(_Inexpressible_(pDefines->Name != NULL)) CONST D3D_SHADER_MACRO* p_defines = nullptr,
				_In_opt_ ID3DInclude* p_include = D3D_COMPILE_STANDARD_FILE_INCLUDE,
				_In_ UINT flag0 = 0, _In_ UINT flag1 = 0
			) noexcept : Shader(vs, ps, ds, {}, {}, input_desc, p_defines, p_include, flag0, flag1) {}
			Shader(
				const SHADER::Desc& vs, const SHADER::Desc& ps, const SHADER::Desc& ds, const SHADER::Desc& hs,
				const std::vector<D3D12_INPUT_ELEMENT_DESC>& input_desc,
				_In_reads_opt_(_Inexpressible_(pDefines->Name != NULL)) CONST D3D_SHADER_MACRO* p_defines = nullptr,
				_In_opt_ ID3DInclude* p_include = D3D_COMPILE_STANDARD_FILE_INCLUDE,
				_In_ UINT flag0 = 0, _In_ UINT flag1 = 0
			) noexcept : Shader(vs, ps, ds, hs, {}, input_desc, p_defines, p_include, flag0, flag1) {}
			Shader(
				const SHADER::Desc& vs, const SHADER::Desc& ps, const SHADER::Desc& ds, const SHADER::Desc& hs, const SHADER::Desc& gs,
				const std::vector<D3D12_INPUT_ELEMENT_DESC>& input_desc,
				_In_reads_opt_(_Inexpressible_(pDefines->Name != NULL)) CONST D3D_SHADER_MACRO* p_defines = nullptr,
				_In_opt_ ID3DInclude* p_include = D3D_COMPILE_STANDARD_FILE_INCLUDE,
				_In_ UINT flag0 = 0, _In_ UINT flag1 = 0
			) noexcept;*/

			Shader(
				const std::shared_ptr<DXC> dxc,
				const SHADER::Desc& vs,
				const std::vector<D3D12_INPUT_ELEMENT_DESC>& input_desc
			) noexcept : Shader(dxc, vs, {}, {}, {}, {}, input_desc) {}
			Shader(
				const std::shared_ptr<DXC> dxc,
				const SHADER::Desc& vs, const SHADER::Desc& ps,
				const std::vector<D3D12_INPUT_ELEMENT_DESC>& input_desc
			) noexcept : Shader(dxc, vs, ps, {}, {}, {}, input_desc) {}
			Shader(
				const std::shared_ptr<DXC> dxc,
				const SHADER::Desc& vs, const SHADER::Desc& ps, const SHADER::Desc& ds,
				const std::vector<D3D12_INPUT_ELEMENT_DESC>& input_desc
			) noexcept : Shader(dxc, vs, ps, ds, {}, {}, input_desc) {}
			Shader(
				const std::shared_ptr<DXC> dxc,
				const SHADER::Desc& vs, const SHADER::Desc& ps, const SHADER::Desc& ds, const SHADER::Desc& hs,
				const std::vector<D3D12_INPUT_ELEMENT_DESC>& input_desc
			) noexcept : Shader(dxc, vs, ps, ds, hs, {}, input_desc) {}
			Shader(
				const std::shared_ptr<DXC> dxc,
				const SHADER::Desc& vs, const SHADER::Desc& ps, const SHADER::Desc& ds, const SHADER::Desc& hs, const SHADER::Desc& gs,
				const std::vector<D3D12_INPUT_ELEMENT_DESC>& input_desc
			) noexcept;
		};

		class ShaderCS
		{
		private:
			ComPtr<ID3DBlob> m_cs;
			//ComPtr<IDxcBlob> m_cs;
		public:
			/*ShaderCS(
				const SHADER::Desc& cs,
				_In_reads_opt_(_Inexpressible_(pDefines->Name != NULL)) CONST D3D_SHADER_MACRO* p_defines = nullptr,
				_In_opt_ ID3DInclude* p_include = D3D_COMPILE_STANDARD_FILE_INCLUDE,
				_In_ UINT flag0 = 0, _In_ UINT flag1 = 0
			) noexcept;*/
			const ComPtr<ID3DBlob>& GetCS() const noexcept { return m_cs; }
			ShaderCS(
				const std::shared_ptr<DXC> dxc,
				const SHADER::Desc& cs
			) noexcept;
			void GetDesc(D3D12_COMPUTE_PIPELINE_STATE_DESC* desc) const noexcept;
		};
	}
}

namespace KGL
{
	inline namespace DX12
	{
		inline void Shader::GetDesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc) const noexcept
		{
			assert(desc && "[ Shader::GetDesc ] DESC ‚ª nullptr");
			if (m_vs)
			{
				desc->VS.pShaderBytecode = m_vs->GetBufferPointer();
				desc->VS.BytecodeLength = m_vs->GetBufferSize();
			}
			if (m_ps)
			{
				desc->PS.pShaderBytecode = m_ps->GetBufferPointer();
				desc->PS.BytecodeLength = m_ps->GetBufferSize();
			}
			if (m_ds)
			{
				desc->DS.pShaderBytecode = m_ds->GetBufferPointer();
				desc->DS.BytecodeLength = m_ds->GetBufferSize();
			}
			if (m_hs)
			{
				desc->HS.pShaderBytecode = m_hs->GetBufferPointer();
				desc->HS.BytecodeLength = m_hs->GetBufferSize();
			}
			if (m_gs)
			{
				desc->GS.pShaderBytecode = m_gs->GetBufferPointer();
				desc->GS.BytecodeLength = m_gs->GetBufferSize();
			}
			desc->InputLayout.NumElements = SCAST<UINT>(m_input_desc.size());
			desc->InputLayout.pInputElementDescs = m_input_desc.data();
		}

		inline void ShaderCS::GetDesc(D3D12_COMPUTE_PIPELINE_STATE_DESC* desc) const noexcept
		{
			assert(desc && "[ Shader::GetDesc ] DESC ‚ª nullptr");
			if (m_cs)
			{
				desc->CS.pShaderBytecode = m_cs->GetBufferPointer();
				desc->CS.BytecodeLength = m_cs->GetBufferSize();
			}
		}
	}
}
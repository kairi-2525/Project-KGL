#pragma once

#include "../Shader.hpp"
#include <map>
#include <string>

namespace KGL
{
	inline namespace DX12
	{
		namespace DXR
		{
			struct ShaderDesc
			{
				std::filesystem::path	hlsl;
				std::string				version;
				std::vector<std::string> entry_points;
			};

			struct SignatureDesc
			{
				ShaderDesc							shader;
				std::vector<D3D12_ROOT_PARAMETER>	root_params;
			};

			using SignatureList = std::map<std::string, SignatureDesc>;

			class Signature
			{
			public:
				struct Data
				{
					ComPtr<ID3DBlob>			shader;
					std::vector<std::wstring>	entry_points;
					ComPtr<ID3D12RootSignature> rs;
				};
			private:
				std::map<std::string, Data>	m_data;
			public:
				const std::map<std::string, Data>& GetData() const noexcept { return m_data; }

				Signature(
					ComPtrC<ID3D12Device> device,
					const std::shared_ptr<DXC>& dxc,
					const SignatureList& signatures
				) noexcept;
			};
		}
	}
}
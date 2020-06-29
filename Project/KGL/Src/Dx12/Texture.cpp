#include <Dx12/Texture.hpp>
#include <functional>

#include <DirectXTex\DirectXTex.h>
#pragma comment(lib, "DirectXTex.lib")

using namespace KGL;
using namespace DirectX;

using LoadLambda_t = std::function<
	HRESULT(const std::filesystem::path& path, TexMetadata*, ScratchImage&)>;

static const LoadLambda_t LoadDefault = [](const std::filesystem::path& path, TexMetadata* meta, ScratchImage& img)
	->HRESULT
	{
		return LoadFromWICFile(path.wstring().c_str(), WIC_FLAGS_NONE, meta, img);
	};

static const std::map<const std::string, LoadLambda_t> TEX_LOAD_LAMBDA_TBL =
	{
		{ ".SPH", LoadDefault },
		{ ".SPA", LoadDefault },
		{ ".BMP", LoadDefault },
		{ ".PNG", LoadDefault },
		{ ".JPG", LoadDefault },
		{ ".JPG", LoadDefault },
		{ ".TGA", [](const std::filesystem::path& path, TexMetadata* meta, ScratchImage& img)
			->HRESULT
			{
				return LoadFromTGAFile(path.wstring().c_str(), meta, img);
			}
		},
		{ ".DDS", [](const std::filesystem::path& path, TexMetadata* meta, ScratchImage& img)
			->HRESULT
			{
				return LoadFromDDSFile(path.wstring().c_str(), DDS_FLAGS_NONE, meta, img);
			}
		},
	};
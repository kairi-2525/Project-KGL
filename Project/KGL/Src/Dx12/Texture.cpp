#include <Dx12/Texture.hpp>
#include <functional>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>

#include <DirectXTex/DirectXTex.h>
#include <DirectXTex/d3dx12.h>
#pragma comment(lib, "DirectXTex.lib")

#include <Helper/Cast.hpp>
#include <intrin.h>

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

bool TextureManager::GetResource(const std::filesystem::path& path,
	ComPtr<ID3D12Resource>* resource) const noexcept
{
	assert(resource && "TextureManager::GetResourceで無効な値の入力");
	auto itr = m_resources.find(path.string());
	if (itr != m_resources.end())
	{
		*resource = itr->second;
		return true;
	}
	return false;
}

bool TextureManager::SetResource(const std::filesystem::path& path,
	const ComPtr<ID3D12Resource>& resource) noexcept
{
	assert(resource && "TextureManager::SetResourceで無効な値の入力");
	if (m_resources.count(path) == 0)
	{
		m_resources[path] = resource;
		return true;
	}
	return false;
}

HRESULT Texture::Create(const ComPtr<ID3D12Device>& device,
	const std::filesystem::path& path, TextureManager* mgr) noexcept
{
	m_path = path;
	TexMetadata metadata = {};
	ScratchImage scratch_img = {};

	if (mgr)
	{
		if (mgr->GetResource(m_path, &m_buffer))
			return S_OK;
	}

	if (!device)
	{
		assert(!"デバイスにNULLが渡されました。");
		return E_FAIL;
	}

	std::string extension = m_path.extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), static_cast<int (*)(int)>(&std::toupper));
	assert(TEX_LOAD_LAMBDA_TBL.count(extension) != 0u && "非対応の拡張子が入力されました。");

	HRESULT hr = TEX_LOAD_LAMBDA_TBL.at(extension)(
		m_path,
		&metadata,
		scratch_img
		);
	try
	{
		if (FAILED(hr) && (hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) && hr != HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)))
			throw std::runtime_error(
				"["
				+ m_path.string()
				+ "] の読み込み中に原因不明のエラーが発生しました。"
			);
		else if (FAILED(hr)) return hr;
	}
	catch (std::runtime_error& exception)
	{
		RuntimeErrorStop(exception);
	}

	// 生データの抽出
	auto img = scratch_img.GetImage(0, 0, 0);

	// WriteToSubresourceで転送する用のヒープ設定
	D3D12_HEAP_PROPERTIES tex_heap_prop = {};
	tex_heap_prop.Type = D3D12_HEAP_TYPE_CUSTOM;
	tex_heap_prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	tex_heap_prop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	tex_heap_prop.CreationNodeMask = 0;		// 単一アダプタのため
	tex_heap_prop.VisibleNodeMask = 0;		// 単一アダプタのため

	auto res_desc =
		CD3DX12_RESOURCE_DESC::Tex2D(
			metadata.format,
			metadata.width,
			SCAST<UINT>(metadata.height),
			SCAST<UINT16>(metadata.arraySize),
			SCAST<UINT16>(metadata.mipLevels)
		);
	res_desc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);

	// バッファ作成

	hr = device->CreateCommittedResource(
		&tex_heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(m_buffer.ReleaseAndGetAddressOf())
	);
	RCHECK(FAILED(hr), "CreateCommittedResourceに失敗", hr);

	if (mgr)
	{
		mgr->SetResource(m_path, m_buffer);
	}

	hr = m_buffer->WriteToSubresource(
		0u,
		nullptr,
		img->pixels,
		SCAST<UINT>(img->rowPitch),
		SCAST<UINT>(img->slicePitch)
	);
	RCHECK(FAILED(hr), "WriteToSubresourceに失敗", hr);

	m_buffer->SetName(m_path.wstring().c_str());
	return hr;
}

HRESULT Texture::Create(const ComPtr<ID3D12Device>& device,
	UCHAR r, UCHAR g, UCHAR b, UCHAR a, TextureManager* mgr) noexcept
{
	{
#pragma warning( disable : 4293 )
		std::stringstream ss;
		UINT32 rgba = (r << 24) | (g << 16) | (b << 8) | a;
		ss << std::hex << rgba;
		m_path = ss.str();
#pragma warning( default : 4293 )
	}

	if (mgr)
	{
		if (mgr->GetResource(m_path, &m_buffer))
			return S_OK;
	}

	if (!device)
	{
		assert(!"デバイスにNULLが渡されました。");
		return E_FAIL;
	}

	D3D12_HEAP_PROPERTIES tex_heap_prop = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0
	);

	auto res_desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4);

	auto hr = device->CreateCommittedResource(
		&tex_heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(m_buffer.ReleaseAndGetAddressOf())
	);
	RCHECK(FAILED(hr), "CreateCommittedResourceに失敗", hr);
	if (mgr) mgr->SetResource(m_path, m_buffer);

	std::vector<UCHAR> data(4 * 4 * 4);	 // 色 * 幅 * 高さ
	// 全部255で埋める
	const size_t data_size = data.size();
	size_t j = 0u;
	for (size_t i = 0u; i < data_size; i++)
	{
		switch (j)
		{
			case 0: data[i] = r; break;
			case 1: data[i] = g; break;
			case 2: data[i] = b; break;
			case 3: data[i] = a; break;
		}
		j = j == 3 ? 0 : j + 1;
	}
	// データ送信
	hr = m_buffer->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4u * 4u,
		SCAST<UINT>(data.size())
	);
	RCHECK(FAILED(hr), "WriteToSubresourceに失敗", hr);
	return hr;
}

HRESULT Texture::Create(const ComPtr<ID3D12Device>& device,
	UCHAR tr, UCHAR tg, UCHAR tb, UCHAR ta,
	UCHAR br, UCHAR bg, UCHAR bb, UCHAR ba, UINT16 height, TextureManager* mgr) noexcept
{
	{
#pragma warning( disable : 4293 )
		std::stringstream ss;
		UINT32 top = (tr << 0xff) | (tg << 16) | (tb << 8) | ta;
		UINT32 bottom = (br << 0xff) | (bg << 16) | (bb << 8) | ba;
		ss << std::setw(8) << std::setfill('0') << std::hex << top << "_" << std::setw(8) << std::setfill('0') << std::hex << bottom;
		m_path = ss.str() + "_" + std::to_string(height);
	}

	if (mgr)
	{
		if (mgr->GetResource(m_path, &m_buffer))
			return S_OK;
	}

	if (!device)
	{
		assert(!"デバイスにNULLが渡されました。");
		return E_FAIL;
	}

	D3D12_HEAP_PROPERTIES tex_heap_prop = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0
	);

	auto res_desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 4, height);

	auto hr = device->CreateCommittedResource(
		&tex_heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(m_buffer.ReleaseAndGetAddressOf())
	);
	RCHECK(FAILED(hr), "テクスチャの読み込みに失敗", hr);
	if (mgr) mgr->SetResource(m_path, m_buffer);

	std::vector<UINT32> data(4u * height);
	auto itr = data.begin();
	UINT32 cr = tr, cg = tg, cb = tb, ca = ta;
	UINT i = 0u;
	for (; itr != data.end(); itr += 4)
	{
		cr = tr + SCAST<INT8>(((SCAST<FLOAT>(br) - tr) / (height - 1)) * i);
		cg = tg + SCAST<INT8>(((SCAST<FLOAT>(bg) - tg) / (height - 1)) * i);
		cb = tb + SCAST<INT8>(((SCAST<FLOAT>(bb) - tb) / (height - 1)) * i);
		ca = ta + SCAST<INT8>(((SCAST<FLOAT>(ba) - ta) / (height - 1)) * i);

		//UINT32 col = (cr << 24) | (cg << 16) | (cb << 8) | ca;
		//col = _byteswap_ulong(col);

		// UINT32 col = (ca << 24) | RGB(cr, cb, cg);

		UINT32 col = (ca << 24) | (cb << 16) | (cg << 8) | cr;

		std::fill(itr, itr + 4, col);

		i++;
	}
#pragma warning( default : 4293 )
	// データ送信
	hr = m_buffer->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4u * sizeof(UINT32),
		SCAST<UINT>(data.size())
	);
	RCHECK(FAILED(hr), "WriteToSubresourceに失敗", hr);
	return hr;
}

// フォーマット指定空テクスチャ
HRESULT Texture::Create(
	ComPtr<ID3D12Device> device,
	const D3D12_RESOURCE_DESC& res_desc,
	const D3D12_CLEAR_VALUE& clear_value,
	D3D12_RESOURCE_STATES res_state
) noexcept
{
	if (m_path.empty()) m_path = "noname";

	auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	// バッファ作成
	auto hr = device->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,	
		res_state,
		&clear_value,
		IID_PPV_ARGS(m_buffer.ReleaseAndGetAddressOf())
	);
	RCHECK(FAILED(hr), "CreateCommittedResourceに失敗", hr);
	return hr;
}

// 作成済みのテクスチャをもとに作成
HRESULT Texture::Create(const ComPtr<ID3D12Device>& device,
	const ComPtr<ID3D12Resource>& resource, const DirectX::XMFLOAT4& clear_value) noexcept
{
	if (m_path.empty()) m_path = "noname";
	HRESULT hr = S_OK;
	RCHECK(!resource, "不正なResourceが渡されました", E_FAIL);
	auto res_desc = resource->GetDesc();
	auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	auto clear_value_dx = CD3DX12_CLEAR_VALUE(res_desc.Format, (FLOAT*)&clear_value);

	hr = device->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clear_value_dx,
		IID_PPV_ARGS(m_buffer.ReleaseAndGetAddressOf())
	);
	RCHECK(FAILED(hr), "CreateCommittedResourceに失敗", hr);
	return hr;
}
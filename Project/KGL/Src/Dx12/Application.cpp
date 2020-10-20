#include <Dx12/Application.hpp>
#include <cassert>
#include <Helper/ThrowAssert.hpp>
#include <Helper/Cast.hpp>
#include <Dx12/SetName.hpp>
#include <DirectXTex/d3dx12.h>

using namespace KGL;

// 垂直同期サポートの確認
bool Application::CheckTearingSupport()
{
	BOOL allow_tearing = FALSE;
	// Rather than create the DXGI 1.5 factory interface directly, we create the
			// DXGI 1.4 interface and query for the 1.5 interface. This is to enable the 
			// graphics debugging tools which will not support the 1.5 factory interface 
			// until a future update.
	ComPtr<IDXGIFactory4> factory4;
	if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
	{
		ComPtr<IDXGIFactory5> factory5;
		if (SUCCEEDED(factory4.As(&factory5)))
		{
			if (FAILED(factory5->CheckFeatureSupport(
				DXGI_FEATURE_PRESENT_ALLOW_TEARING,
				&allow_tearing, sizeof(allow_tearing))))
			{
				allow_tearing = FALSE;
			}
		}
	}

	return allow_tearing == TRUE;
}

// DXRサポートの確認
bool Application::CheckDXRSupport(ComPtrC<ID3D12Device> device)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 feature_support_data = {};
	HRESULT hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &feature_support_data, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
	if (SUCCEEDED(hr) && D3D12_RAYTRACING_TIER_NOT_SUPPORTED != feature_support_data.RaytracingTier)
	{
		return true;
	}
	return false;
}

Application::Application(HWND hwnd, bool debug_layer, bool gbv) noexcept
{
	HRESULT hr = S_OK;
	m_tearing_support = CheckTearingSupport();
	{
		ComPtr<IDXGIFactory6> factory;
		hr = CreateFactory(factory, debug_layer, gbv);
		RCHECK(FAILED(hr), "CreateFactoryに失敗");

		hr = CreateDevice(factory);
		RCHECK(FAILED(hr), "CreateDeviceに失敗");

		m_dxr_support = CheckDXRSupport(m_dev);

		hr = CheckMaxSampleCount();
		RCHECK(FAILED(hr), "CheckMaxSampleCountに失敗");

		hr = CreateSwapchain(factory, hwnd);
		RCHECK(FAILED(hr), "CreateSwapchainに失敗");
		hr = CreateHeaps();
		RCHECK(FAILED(hr), "CreateHeapsに失敗");
	}
}

Application::~Application()
{
	m_cmd_queue->Signal();
	m_cmd_queue->Wait();
}

HRESULT Application::CreateFactory(ComPtr<IDXGIFactory6>& factory, bool debug_layer, bool gbv)
{
	HRESULT hr = S_OK;
	if (debug_layer)
	{
		ComPtr<ID3D12Debug> debug_layer;
		hr = D3D12GetDebugInterface(
			IID_PPV_ARGS(debug_layer.ReleaseAndGetAddressOf()));
		if (FAILED(hr)) return hr;
		debug_layer->EnableDebugLayer();

		// GBVを有効化するとシェーダー関連の調査を行うようになりパフォーマンスに影響を及ぼします。
		if (gbv)
		{
			ComPtr<ID3D12Debug3> debug_gbv;
			debug_layer.As(&debug_gbv);
			if (debug_gbv)
			{
				//GBV(GPU ベースのバリデーション)の有効化
				debug_gbv->SetEnableGPUBasedValidation(true);
			}
		}

		hr = CreateDXGIFactory2(
			DXGI_CREATE_FACTORY_DEBUG,
			IID_PPV_ARGS(factory.ReleaseAndGetAddressOf()));
	}
	else
	{
		hr = CreateDXGIFactory1(
			IID_PPV_ARGS(factory.ReleaseAndGetAddressOf()));
	}
	return hr;
}

HRESULT Application::CreateDevice(ComPtr<IDXGIFactory6> factory) noexcept
{
	HRESULT hr = S_OK;
	// ハードウェアアダプターの検索
	// ここでDirextX12に対応しているかをチェックする。
	ComPtr<IDXGIAdapter1> use_adapter;
	D3D_FEATURE_LEVEL use_lv = {};
	{
		UINT adapter_index{};
		ComPtr<IDXGIAdapter1> set_adapter;
		D3D_FEATURE_LEVEL levels[] =
		{
			D3D_FEATURE_LEVEL_12_1,
			D3D_FEATURE_LEVEL_12_0,
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0
		};
		ComPtr<IDXGIAdapter1> top_adapter;
		UINT8 top_lv = 0u;
		SIZE_T max_memory_size = 0u;
		constexpr UINT8 max_lv = SCAST<UINT8>(std::size(levels));

		while (DXGI_ERROR_NOT_FOUND != (hr = factory->EnumAdapters1(adapter_index++, &set_adapter)))
		{
			DXGI_ADAPTER_DESC1 desc{};
			set_adapter->GetDesc1(&desc);
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				continue;
			//DirectX12が使えるか確認
			UINT8 lv_count = SCAST<UINT8>(std::size(levels));
			for (auto& lv : levels)
			{
				hr = D3D12CreateDevice(set_adapter.Get(), lv, __uuidof(ID3D12Device), nullptr);
				if (SUCCEEDED(hr))
				{
					if (top_lv <= lv_count && max_memory_size < desc.DedicatedVideoMemory)
					{
						top_lv = lv_count;
						use_lv = lv;
						max_memory_size = desc.DedicatedVideoMemory;
						set_adapter.As(&top_adapter);
						m_desc = desc;
						break;
					}
				}
				lv_count--;
			}
		}
		try
		{
			if (FAILED(hr) && !top_adapter) throw std::runtime_error("DirectX12に対応するハードウェアアダプターが見つかりませんでした。\nこの環境ではDirectX12を実行できません。");
		}
		catch (std::runtime_error& exception)
		{
			RuntimeErrorStop(exception);
		}
		//使用するアダプターをセット
		top_adapter.As(&use_adapter);
	}
	hr = D3D12CreateDevice(use_adapter.Get(), use_lv, IID_PPV_ARGS(m_dev.ReleaseAndGetAddressOf()));
	assert(SUCCEEDED(hr) && "デバイスの生成に失敗！");
	return hr;
}

HRESULT Application::CreateSwapchain(ComPtr<IDXGIFactory6> factory, HWND hwnd) noexcept
{
	HRESULT hr = S_OK;
	UINT width, height;
	{
		RECT rect;
		RCHECK(!GetClientRect(hwnd, &rect), "HWNDからクライアント領域の取得に失敗！", E_FAIL);
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;
	}
	{	// コマンドキューの生成
		D3D12_COMMAND_QUEUE_DESC cmd_queue_desc = {};
		// タイムアウトなし
		cmd_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		// アダプターを１つしか使わないときは０でよい
		cmd_queue_desc.NodeMask = 0;
		// プライオリティは特に指定なし
		cmd_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		// コマンドリストと合わせる
		cmd_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		m_cmd_queue = std::make_shared<CommandQueue>(m_dev, cmd_queue_desc);
		RCHECK(FAILED(hr), "コマンドキューの生成に失敗！", hr);
	}
	{	// スワップチェインの生成
		DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {};
		swapchain_desc.Width = width;
		swapchain_desc.Height = height;
		swapchain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapchain_desc.Stereo = false;		// 3Dディスプレイのステレオモード
		swapchain_desc.SampleDesc.Count = 1;
		swapchain_desc.SampleDesc.Quality = 0;
		swapchain_desc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
		swapchain_desc.BufferCount = 2;
		// バックバッファーは伸び縮み可能
		swapchain_desc.Scaling = DXGI_SCALING_STRETCH;
		// フリップ後は速やかに破棄
		swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		// 特に指定なし
		swapchain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		// ウィンドウ⇔フルスクリーン切り替え可能
		swapchain_desc.Flags = 
			DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
			| (m_tearing_support ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0);

		hr = factory->CreateSwapChainForHwnd(
			m_cmd_queue->Data().Get(),
			hwnd,
			&swapchain_desc,
			nullptr,
			nullptr,
			(IDXGISwapChain1**)m_swapchain.ReleaseAndGetAddressOf()
		);
		RCHECK(FAILED(hr), "スワップチェインの生成に失敗！", hr);

		// Alt+Enterによる全画面遷移をできないようにする
		hr = factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
		RCHECK(FAILED(hr), "Alt+Enterによる全画面遷移の設定に失敗！", hr);
	}
	return hr;
}

HRESULT Application::CreateHeaps() noexcept
{
	HRESULT hr = S_OK;
	{
		m_rtv_heap = std::make_shared<DescriptorManager>(m_dev, 100u, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	}
	{
		DXGI_SWAP_CHAIN_DESC swc_desc = {};
		hr = m_swapchain->GetDesc(&swc_desc);

		m_rtv_buffers.resize(swc_desc.BufferCount, nullptr);

		D3D12_RENDER_TARGET_VIEW_DESC* rtv_desc_ptr = nullptr;
#ifdef USE_SRGB
		// SRGB用レンダーターゲットビュー設定
		D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
		rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // ガンマ補正あり(sRGB)
		rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtv_desc_ptr = &rtv_desc;
#else
		D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
		rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtv_desc_ptr = &rtv_desc;
#endif
		m_rtv_handles.resize(swc_desc.BufferCount);
		for (UINT idx = 0; idx < swc_desc.BufferCount; idx++)
		{
			hr = m_swapchain->GetBuffer(idx, IID_PPV_ARGS(m_rtv_buffers[idx].ReleaseAndGetAddressOf()));
			RCHECK(FAILED(hr), "RTVのバッファー確保に失敗", hr);
			SetName(m_rtv_buffers[idx], RCAST<INT_PTR>(this), L" Application", L"RTV");

			m_rtv_handles[idx] = m_rtv_heap->Alloc();
			m_dev->CreateRenderTargetView(
				m_rtv_buffers[idx].Get(),
				rtv_desc_ptr,
				m_rtv_handles[idx].Cpu()
			);
		}
	}
	{
		DXGI_SWAP_CHAIN_DESC1 swc_desc = {};
		m_swapchain->GetDesc1(&swc_desc);
		D3D12_RESOURCE_DESC depth_res_desc = {};
		depth_res_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;		// 2次元のテクスチャデータ
		depth_res_desc.Width = swc_desc.Width;								// 幅と高さはレンダーターゲットと同じ
		depth_res_desc.Height = swc_desc.Height;
		depth_res_desc.DepthOrArraySize = 1;								// テクスチャ配列でも３D配列でもない
		depth_res_desc.Format = DXGI_FORMAT_R32G8X24_TYPELESS;					// 深度値書き込み用フォーマット
		depth_res_desc.SampleDesc.Count = 1;								// サンプルは１ピクセル当たり１つ
		depth_res_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;		// デプスステンシルとして使用
		depth_res_desc.MipLevels = 1;
		// 深度値用ヒーププロパティ
		D3D12_HEAP_PROPERTIES depth_heap_prop = {};
		depth_heap_prop.Type = D3D12_HEAP_TYPE_DEFAULT;
		depth_heap_prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		depth_heap_prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

		//CD3DX12_CLEAR_VALUE depth_clear_value(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);
		D3D12_CLEAR_VALUE depth_clear_value = {};
		depth_clear_value.DepthStencil.Depth = 1.0f;		// 深さの最大値でクリア
		depth_clear_value.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

		hr = m_dev->CreateCommittedResource(
			&depth_heap_prop,
			D3D12_HEAP_FLAG_NONE,
			&depth_res_desc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,	// 深度書き込みに使用
			&depth_clear_value,
			IID_PPV_ARGS(m_depth_buff.ReleaseAndGetAddressOf())
		);
		RCHECK(FAILED(hr), "DSVのリソース確保に失敗", hr);
		SetName(m_depth_buff, RCAST<INT_PTR>(this), L" Application", L"DSV");

		// 深度のためのディスクリプタヒープ
		D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc = {};
		dsv_heap_desc.NumDescriptors = 1;
		dsv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

		hr = m_dev->CreateDescriptorHeap(
			&dsv_heap_desc, IID_PPV_ARGS(m_dsv_heap.ReleaseAndGetAddressOf())
		);
		RCHECK(FAILED(hr), "DSVのバッファー確保に失敗", hr);

		// 深度ビュー
		D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
		dsv_desc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsv_desc.Flags = D3D12_DSV_FLAG_NONE;	// フラグ無し

		m_dev->CreateDepthStencilView(
			m_depth_buff.Get(),
			&dsv_desc,
			m_dsv_heap->GetCPUDescriptorHandleForHeapStart()
		);
	}
	return hr;
}

HRESULT Application::CheckMaxSampleCount() noexcept
{
	HRESULT hr = S_OK;
	m_max_quality_level = 0u;
	m_max_sample_count = 1u;
	for (UINT sample_count = 1; sample_count <= D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT; sample_count++) {
		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaa_quality_desc{};
		msaa_quality_desc.SampleCount = sample_count;
		msaa_quality_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		msaa_quality_desc.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
		msaa_quality_desc.NumQualityLevels = 0;

		hr = m_dev->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaa_quality_desc, sizeof(msaa_quality_desc));
		RCHECK_HR(hr, "MSAAのSampleCountのチェックに失敗に失敗");
		if (msaa_quality_desc.NumQualityLevels > 0) {
			if (m_max_quality_level <= msaa_quality_desc.NumQualityLevels) {
				m_max_quality_level = msaa_quality_desc.NumQualityLevels;
				m_max_sample_count = sample_count;
			}
		}
	}
	return hr;
}

void Application::SetRtvDsv(ComPtr<ID3D12GraphicsCommandList> cmd_list) const noexcept
{
	RCHECK(!cmd_list, "cmd_list が nullptr");
	auto dsv_handle = m_dsv_heap->GetCPUDescriptorHandleForHeapStart();
	cmd_list->OMSetRenderTargets(1, &m_rtv_handles[m_swapchain->GetCurrentBackBufferIndex()].Cpu(), true, &dsv_handle);
}

void Application::SetRtv(ComPtr<ID3D12GraphicsCommandList> cmd_list) const noexcept
{
	RCHECK(!cmd_list, "cmd_list が nullptr");
	cmd_list->OMSetRenderTargets(1, &m_rtv_handles[m_swapchain->GetCurrentBackBufferIndex()].Cpu(), true, nullptr);
}

void Application::SetDsv(ComPtr<ID3D12GraphicsCommandList> cmd_list) const noexcept
{
	RCHECK(!cmd_list, "cmd_list が nullptr");
	auto dsv_handle = m_dsv_heap->GetCPUDescriptorHandleForHeapStart();
	cmd_list->OMSetRenderTargets(0, nullptr, true, &dsv_handle);
}

void Application::ClearRtv(ComPtr<ID3D12GraphicsCommandList> cmd_list,
	const DirectX::XMFLOAT4& clear_color) const noexcept
{
	RCHECK(!cmd_list, "cmd_list が nullptr");

	cmd_list->ClearRenderTargetView(m_rtv_handles[m_swapchain->GetCurrentBackBufferIndex()].Cpu(), (float*)&clear_color, 0, nullptr);
}

void Application::ClearDsv(ComPtr<ID3D12GraphicsCommandList> cmd_list) const noexcept
{
	auto dsv_handle = m_dsv_heap->GetCPUDescriptorHandleForHeapStart();
	cmd_list->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

D3D12_RESOURCE_BARRIER Application::GetRtvResourceBarrier(bool render_target) const noexcept
{
	return render_target ?
		CD3DX12_RESOURCE_BARRIER::Transition(
			m_rtv_buffers[m_swapchain->GetCurrentBackBufferIndex()].Get(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		) :
		CD3DX12_RESOURCE_BARRIER::Transition(
			m_rtv_buffers[m_swapchain->GetCurrentBackBufferIndex()].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT
		);
}

HRESULT Application::GetDevice5(ComPtr<ID3D12Device5>* p_dev5) const noexcept
{
	RCHECK(!p_dev5, "p_dev5 が nullptr", E_FAIL);
	HRESULT hr = m_dev->QueryInterface(IID_PPV_ARGS(p_dev5->GetAddressOf()));
	return hr;
}
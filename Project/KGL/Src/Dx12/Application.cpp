#include <Dx12/Application.hpp>
#include <cassert>
#include <Helper/ThrowAssert.hpp>
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

		hr = CreateSwapchain(factory, hwnd);
		RCHECK(FAILED(hr), "CreateSwapchainに失敗");
		hr = CreateHeaps();
		RCHECK(FAILED(hr), "CreateHeapsに失敗");
	}
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

		while (DXGI_ERROR_NOT_FOUND != (hr = factory->EnumAdapters1(adapter_index++, &set_adapter)))
		{
			DXGI_ADAPTER_DESC1 desc{};
			set_adapter->GetDesc1(&desc);
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				continue;
			//DirectX12が使えるか確認
			for (auto& lv : levels)
			{
				hr = D3D12CreateDevice(set_adapter.Get(), lv, __uuidof(ID3D12Device), nullptr);
				if (SUCCEEDED(hr))
				{
					use_lv = lv;
					break;
				}
			}
			if (SUCCEEDED(hr)) break;
		}
		try
		{
			if (FAILED(hr)) throw std::runtime_error("DirectX12に対応するハードウェアアダプターが見つかりませんでした。\nこの環境ではDirectX12を実行できません。");
		}
		catch (std::runtime_error& exception)
		{
			RuntimeErrorStop(exception);
		}
		//使用するアダプターをセット
		set_adapter.As(&use_adapter);
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

		hr = m_dev->CreateCommandQueue(&cmd_queue_desc, IID_PPV_ARGS(m_cmd_queue.ReleaseAndGetAddressOf()));
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
			| m_tearing_support ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

		hr = factory->CreateSwapChainForHwnd(
			m_cmd_queue.Get(),
			hwnd,
			&swapchain_desc,
			nullptr,
			nullptr,
			(IDXGISwapChain1**)m_swapchain.ReleaseAndGetAddressOf()
		);
		RCHECK(FAILED(hr), "スワップチェインの生成に失敗！", hr);
	}
	return hr;
}

HRESULT Application::CreateHeaps()
{
	HRESULT hr = S_OK;
	{
		D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
		// レンダーターゲットビューなのでRTV
		heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		heap_desc.NodeMask = 0;
		// 表裏の２つ
		heap_desc.NumDescriptors = 2;
		// 特に指定なし(シェーダー側から見る必要がないため)
		heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		hr = m_dev->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(m_rtv_heap.ReleaseAndGetAddressOf()));
		RCHECK(FAILED(hr), "RTVのディスクリプタヒープの生成に失敗！", hr);
	}
	{
		DXGI_SWAP_CHAIN_DESC swc_desc = {};
		hr = m_swapchain->GetDesc(&swc_desc);

		m_rtv_buffers.resize(swc_desc.BufferCount, nullptr);

		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_rtv_heap->GetCPUDescriptorHandleForHeapStart());
		const UINT increment_size = m_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		D3D12_RENDER_TARGET_VIEW_DESC* rtv_desc_ptr = nullptr;
#ifdef USE_SRGB
		// SRGB用レンダーターゲットビュー設定
		D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
		rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // ガンマ補正あり(sRGB)
		rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtv_desc_ptr = &rtv_desc;
#endif
		for (UINT idx = 0; idx < swc_desc.BufferCount; idx++)
		{
			hr = m_swapchain->GetBuffer(idx, IID_PPV_ARGS(m_rtv_buffers[idx].ReleaseAndGetAddressOf()));
			RCHECK(FAILED(hr), "RTVのバッファー確保に失敗", hr);

			m_dev->CreateRenderTargetView(
				m_rtv_buffers[idx].Get(),
				rtv_desc_ptr,
				handle
			);
			handle.Offset(increment_size);
		}
	}
	{
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
		dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
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
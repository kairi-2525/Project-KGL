#include <Dx12/Application.hpp>
#include <cassert>
#include <Helper/ThrowAssert.hpp>
#include <Helper/Cast.hpp>
#include <Dx12/SetName.hpp>
#include <DirectXTex/d3dx12.h>

using namespace KGL;

// ���������T�|�[�g�̊m�F
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

// DXR�T�|�[�g�̊m�F
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
		RCHECK(FAILED(hr), "CreateFactory�Ɏ��s");

		hr = CreateDevice(factory);
		RCHECK(FAILED(hr), "CreateDevice�Ɏ��s");

		m_dxr_support = CheckDXRSupport(m_dev);

		hr = CheckMaxSampleCount();
		RCHECK(FAILED(hr), "CheckMaxSampleCount�Ɏ��s");

		hr = CreateSwapchain(factory, hwnd);
		RCHECK(FAILED(hr), "CreateSwapchain�Ɏ��s");
		hr = CreateHeaps();
		RCHECK(FAILED(hr), "CreateHeaps�Ɏ��s");
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

		// GBV��L��������ƃV�F�[�_�[�֘A�̒������s���悤�ɂȂ�p�t�H�[�}���X�ɉe�����y�ڂ��܂��B
		if (gbv)
		{
			ComPtr<ID3D12Debug3> debug_gbv;
			debug_layer.As(&debug_gbv);
			if (debug_gbv)
			{
				//GBV(GPU �x�[�X�̃o���f�[�V����)�̗L����
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
	// �n�[�h�E�F�A�A�_�v�^�[�̌���
	// ������DirextX12�ɑΉ����Ă��邩���`�F�b�N����B
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
			//DirectX12���g���邩�m�F
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
			if (FAILED(hr) && !top_adapter) throw std::runtime_error("DirectX12�ɑΉ�����n�[�h�E�F�A�A�_�v�^�[��������܂���ł����B\n���̊��ł�DirectX12�����s�ł��܂���B");
		}
		catch (std::runtime_error& exception)
		{
			RuntimeErrorStop(exception);
		}
		//�g�p����A�_�v�^�[���Z�b�g
		top_adapter.As(&use_adapter);
	}
	hr = D3D12CreateDevice(use_adapter.Get(), use_lv, IID_PPV_ARGS(m_dev.ReleaseAndGetAddressOf()));
	assert(SUCCEEDED(hr) && "�f�o�C�X�̐����Ɏ��s�I");
	return hr;
}

HRESULT Application::CreateSwapchain(ComPtr<IDXGIFactory6> factory, HWND hwnd) noexcept
{
	HRESULT hr = S_OK;
	UINT width, height;
	{
		RECT rect;
		RCHECK(!GetClientRect(hwnd, &rect), "HWND����N���C�A���g�̈�̎擾�Ɏ��s�I", E_FAIL);
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;
	}
	{	// �R�}���h�L���[�̐���
		D3D12_COMMAND_QUEUE_DESC cmd_queue_desc = {};
		// �^�C���A�E�g�Ȃ�
		cmd_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		// �A�_�v�^�[���P�����g��Ȃ��Ƃ��͂O�ł悢
		cmd_queue_desc.NodeMask = 0;
		// �v���C�I���e�B�͓��Ɏw��Ȃ�
		cmd_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		// �R�}���h���X�g�ƍ��킹��
		cmd_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		m_cmd_queue = std::make_shared<CommandQueue>(m_dev, cmd_queue_desc);
		RCHECK(FAILED(hr), "�R�}���h�L���[�̐����Ɏ��s�I", hr);
	}
	{	// �X���b�v�`�F�C���̐���
		DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {};
		swapchain_desc.Width = width;
		swapchain_desc.Height = height;
		swapchain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapchain_desc.Stereo = false;		// 3D�f�B�X�v���C�̃X�e���I���[�h
		swapchain_desc.SampleDesc.Count = 1;
		swapchain_desc.SampleDesc.Quality = 0;
		swapchain_desc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
		swapchain_desc.BufferCount = 2;
		// �o�b�N�o�b�t�@�[�͐L�яk�݉\
		swapchain_desc.Scaling = DXGI_SCALING_STRETCH;
		// �t���b�v��͑��₩�ɔj��
		swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		// ���Ɏw��Ȃ�
		swapchain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		// �E�B���h�E�̃t���X�N���[���؂�ւ��\
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
		RCHECK(FAILED(hr), "�X���b�v�`�F�C���̐����Ɏ��s�I", hr);

		// Alt+Enter�ɂ��S��ʑJ�ڂ��ł��Ȃ��悤�ɂ���
		hr = factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
		RCHECK(FAILED(hr), "Alt+Enter�ɂ��S��ʑJ�ڂ̐ݒ�Ɏ��s�I", hr);
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
		// SRGB�p�����_�[�^�[�Q�b�g�r���[�ݒ�
		D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
		rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // �K���}�␳����(sRGB)
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
			RCHECK(FAILED(hr), "RTV�̃o�b�t�@�[�m�ۂɎ��s", hr);
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
		depth_res_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;		// 2�����̃e�N�X�`���f�[�^
		depth_res_desc.Width = swc_desc.Width;								// ���ƍ����̓����_�[�^�[�Q�b�g�Ɠ���
		depth_res_desc.Height = swc_desc.Height;
		depth_res_desc.DepthOrArraySize = 1;								// �e�N�X�`���z��ł��RD�z��ł��Ȃ�
		depth_res_desc.Format = DXGI_FORMAT_R32G8X24_TYPELESS;					// �[�x�l�������ݗp�t�H�[�}�b�g
		depth_res_desc.SampleDesc.Count = 1;								// �T���v���͂P�s�N�Z��������P��
		depth_res_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;		// �f�v�X�X�e���V���Ƃ��Ďg�p
		depth_res_desc.MipLevels = 1;
		// �[�x�l�p�q�[�v�v���p�e�B
		D3D12_HEAP_PROPERTIES depth_heap_prop = {};
		depth_heap_prop.Type = D3D12_HEAP_TYPE_DEFAULT;
		depth_heap_prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		depth_heap_prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

		//CD3DX12_CLEAR_VALUE depth_clear_value(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);
		D3D12_CLEAR_VALUE depth_clear_value = {};
		depth_clear_value.DepthStencil.Depth = 1.0f;		// �[���̍ő�l�ŃN���A
		depth_clear_value.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

		hr = m_dev->CreateCommittedResource(
			&depth_heap_prop,
			D3D12_HEAP_FLAG_NONE,
			&depth_res_desc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,	// �[�x�������݂Ɏg�p
			&depth_clear_value,
			IID_PPV_ARGS(m_depth_buff.ReleaseAndGetAddressOf())
		);
		RCHECK(FAILED(hr), "DSV�̃��\�[�X�m�ۂɎ��s", hr);
		SetName(m_depth_buff, RCAST<INT_PTR>(this), L" Application", L"DSV");

		// �[�x�̂��߂̃f�B�X�N���v�^�q�[�v
		D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc = {};
		dsv_heap_desc.NumDescriptors = 1;
		dsv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

		hr = m_dev->CreateDescriptorHeap(
			&dsv_heap_desc, IID_PPV_ARGS(m_dsv_heap.ReleaseAndGetAddressOf())
		);
		RCHECK(FAILED(hr), "DSV�̃o�b�t�@�[�m�ۂɎ��s", hr);

		// �[�x�r���[
		D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
		dsv_desc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsv_desc.Flags = D3D12_DSV_FLAG_NONE;	// �t���O����

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
		RCHECK_HR(hr, "MSAA��SampleCount�̃`�F�b�N�Ɏ��s�Ɏ��s");
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
	RCHECK(!cmd_list, "cmd_list �� nullptr");
	auto dsv_handle = m_dsv_heap->GetCPUDescriptorHandleForHeapStart();
	cmd_list->OMSetRenderTargets(1, &m_rtv_handles[m_swapchain->GetCurrentBackBufferIndex()].Cpu(), true, &dsv_handle);
}

void Application::SetRtv(ComPtr<ID3D12GraphicsCommandList> cmd_list) const noexcept
{
	RCHECK(!cmd_list, "cmd_list �� nullptr");
	cmd_list->OMSetRenderTargets(1, &m_rtv_handles[m_swapchain->GetCurrentBackBufferIndex()].Cpu(), true, nullptr);
}

void Application::SetDsv(ComPtr<ID3D12GraphicsCommandList> cmd_list) const noexcept
{
	RCHECK(!cmd_list, "cmd_list �� nullptr");
	auto dsv_handle = m_dsv_heap->GetCPUDescriptorHandleForHeapStart();
	cmd_list->OMSetRenderTargets(0, nullptr, true, &dsv_handle);
}

void Application::ClearRtv(ComPtr<ID3D12GraphicsCommandList> cmd_list,
	const DirectX::XMFLOAT4& clear_color) const noexcept
{
	RCHECK(!cmd_list, "cmd_list �� nullptr");

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
	RCHECK(!p_dev5, "p_dev5 �� nullptr", E_FAIL);
	HRESULT hr = m_dev->QueryInterface(IID_PPV_ARGS(p_dev5->GetAddressOf()));
	return hr;
}
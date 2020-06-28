#include <Dx12/Application.hpp>
#include <cassert>
#include <Helper/ThrowAssert.hpp>
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

		hr = CreateSwapchain(factory, hwnd);
		RCHECK(FAILED(hr), "CreateSwapchain�Ɏ��s");
		hr = CreateHeaps();
		RCHECK(FAILED(hr), "CreateHeaps�Ɏ��s");
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

		while (DXGI_ERROR_NOT_FOUND != (hr = factory->EnumAdapters1(adapter_index++, &set_adapter)))
		{
			DXGI_ADAPTER_DESC1 desc{};
			set_adapter->GetDesc1(&desc);
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				continue;
			//DirectX12���g���邩�m�F
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
			if (FAILED(hr)) throw std::runtime_error("DirectX12�ɑΉ�����n�[�h�E�F�A�A�_�v�^�[��������܂���ł����B\n���̊��ł�DirectX12�����s�ł��܂���B");
		}
		catch (std::runtime_error& exception)
		{
			RuntimeErrorStop(exception);
		}
		//�g�p����A�_�v�^�[���Z�b�g
		set_adapter.As(&use_adapter);
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

		hr = m_dev->CreateCommandQueue(&cmd_queue_desc, IID_PPV_ARGS(m_cmd_queue.ReleaseAndGetAddressOf()));
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
			| m_tearing_support ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

		hr = factory->CreateSwapChainForHwnd(
			m_cmd_queue.Get(),
			hwnd,
			&swapchain_desc,
			nullptr,
			nullptr,
			(IDXGISwapChain1**)m_swapchain.ReleaseAndGetAddressOf()
		);
		RCHECK(FAILED(hr), "�X���b�v�`�F�C���̐����Ɏ��s�I", hr);
	}
	return hr;
}

HRESULT Application::CreateHeaps()
{
	HRESULT hr = S_OK;
	{
		D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
		// �����_�[�^�[�Q�b�g�r���[�Ȃ̂�RTV
		heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		heap_desc.NodeMask = 0;
		// �\���̂Q��
		heap_desc.NumDescriptors = 2;
		// ���Ɏw��Ȃ�(�V�F�[�_�[�����猩��K�v���Ȃ�����)
		heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		hr = m_dev->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(m_rtv_heap.ReleaseAndGetAddressOf()));
		RCHECK(FAILED(hr), "RTV�̃f�B�X�N���v�^�q�[�v�̐����Ɏ��s�I", hr);
	}
	{
		DXGI_SWAP_CHAIN_DESC swc_desc = {};
		hr = m_swapchain->GetDesc(&swc_desc);

		m_rtv_buffers.resize(swc_desc.BufferCount, nullptr);

		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_rtv_heap->GetCPUDescriptorHandleForHeapStart());
		const UINT increment_size = m_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		D3D12_RENDER_TARGET_VIEW_DESC* rtv_desc_ptr = nullptr;
#ifdef USE_SRGB
		// SRGB�p�����_�[�^�[�Q�b�g�r���[�ݒ�
		D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
		rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // �K���}�␳����(sRGB)
		rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtv_desc_ptr = &rtv_desc;
#endif
		for (UINT idx = 0; idx < swc_desc.BufferCount; idx++)
		{
			hr = m_swapchain->GetBuffer(idx, IID_PPV_ARGS(m_rtv_buffers[idx].ReleaseAndGetAddressOf()));
			RCHECK(FAILED(hr), "RTV�̃o�b�t�@�[�m�ۂɎ��s", hr);

			m_dev->CreateRenderTargetView(
				m_rtv_buffers[idx].Get(),
				rtv_desc_ptr,
				handle
			);
			handle.Offset(increment_size);
		}
	}
	{
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
		dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
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
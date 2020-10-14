#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <memory>
#include <mutex>

#include <Dx12/Application.hpp>
#include <Base/Window.hpp>
#include <Base/Input.hpp>
#include <Helper/ComPtr.hpp>
#include <Dx12/DescriptorHeap.hpp>
#include <DirectXTex/d3dx12.h>
#include <Helper/ThrowAssert.hpp>
#include <Base/DXC.hpp>

class SceneManager;
struct SceneDesc
{
	std::shared_ptr<KGL::App> app;
	std::shared_ptr<KGL::Window> window;
	std::shared_ptr<KGL::Input> input;
	std::shared_ptr<KGL::DescriptorManager>	imgui_heap_mgr;
	KGL::DescriptorHandle		imgui_handle;
	std::shared_ptr<KGL::DXC>	dxc;
};

class SceneBase
{
public:
	struct SceneBuffers
	{
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX proj;
		DirectX::XMMATRIX light_cam;
		DirectX::XMFLOAT3 eye;	// 視点座標
		float pading;
		DirectX::XMFLOAT3 light_vector;	// ライトベクトル
	};
private:
	bool m_move_allow;
	bool m_loaded;
	std::mutex m_loaded_mutex;
	bool m_load_started;
	std::shared_ptr<SceneBase> m_next_scene;
private:
	HRESULT BaseLoad(SceneDesc desc);
public:
	SceneBase() : m_loaded(false), m_load_started(false), m_move_allow(true) {}
	virtual ~SceneBase() = default;
	HRESULT virtual Load(const SceneDesc& desc) = 0;
	HRESULT virtual Init(const SceneDesc& desc) { return S_OK; }
	HRESULT virtual Update(const SceneDesc& desc, float elapsed_time) = 0;
	HRESULT virtual UnInit(const SceneDesc& desc, std::shared_ptr<SceneBase> next_scene) { return S_OK; }

	bool IsLoaded() noexcept { std::lock_guard<std::mutex> lock(m_loaded_mutex); return m_loaded; };
	bool IsLoadStarted() const noexcept { return m_load_started; }
	template<class _Scene>
	void SetNextScene(const SceneDesc& desc, bool single_thread = false) noexcept
	{
		if (m_next_scene) return;
		m_next_scene = std::make_shared<_Scene>();
		if (!single_thread)
		{
			m_next_scene->m_load_started = true;
			UnInit(desc, m_next_scene);
			std::thread th(&SceneBase::BaseLoad, m_next_scene.get(), desc);
			th.detach();
		}
	}
	void SetMoveSceneFlg(bool allow) noexcept { m_move_allow = allow; };
	bool IsAllowMoveScene() const noexcept { return m_move_allow; }
	const std::shared_ptr<SceneBase>& GetNextScene() noexcept { return m_next_scene; }
};

template <class _Ty>
struct SceneBufferDx12
{
	std::shared_ptr<KGL::DescriptorManager>	desc_mgr;
	KGL::DescriptorHandle					handle;
	KGL::ComPtr<ID3D12Resource>				buff;
	_Ty* mapped_data;
private:
	UINT64 size_in_bytes;
public:
	SceneBufferDx12() : mapped_data(nullptr) {}
	HRESULT Load(const SceneDesc& desc);
	KGL::ComPtrC<ID3D12Resource> Data() const noexcept { return buff; }
	UINT64 SizeInBytes()  const noexcept { return size_in_bytes; }
};

class SceneManager
{
private:
	std::shared_ptr<SceneBase> m_scene;
public:
	template<class _Scene>
	HRESULT Init(const SceneDesc& desc)
	{
		m_scene = std::make_shared<_Scene>();
		auto hr = m_scene->Load(desc);
		if (FAILED(hr)) return hr;
		return m_scene->Init(desc);
	}
	HRESULT UnInit(const SceneDesc& desc, std::shared_ptr<SceneBase> next_scene)
	{
		if (m_scene) return m_scene->UnInit(desc, next_scene);
		return S_OK;
	}
	HRESULT Update(const SceneDesc& desc, float elapsed_time);
	HRESULT SceneChangeUpdate(const SceneDesc& desc);
};

template <class _Ty>
HRESULT SceneBufferDx12<_Ty>::Load(const SceneDesc& desc)
{
	HRESULT hr = S_OK;
	const auto& device = desc.app->GetDevice();

	desc_mgr = std::make_shared<KGL::DescriptorManager>(device, 1u);
	handle = desc_mgr->Alloc();

	size_in_bytes = (sizeof(_Ty) + 0xff) & ~0xff;
	hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size_in_bytes),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(buff.ReleaseAndGetAddressOf())
	);
	RCHECK(FAILED(hr), "CreateCommittedResourceに失敗", hr);
	mapped_data = nullptr;
	hr = buff->Map(0, nullptr, (void**)&mapped_data);
	RCHECK(FAILED(hr), "blur_const_buff->Mapに失敗", hr);

	D3D12_CONSTANT_BUFFER_VIEW_DESC mat_cbv_desc = {};
	mat_cbv_desc.BufferLocation = buff->GetGPUVirtualAddress();
	mat_cbv_desc.SizeInBytes = SCAST<UINT>(size_in_bytes);
	device->CreateConstantBufferView(&mat_cbv_desc, handle.Cpu());

	return hr;
}
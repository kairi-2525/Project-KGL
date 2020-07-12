#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <memory>
#include <mutex>

#include <Dx12/Application.hpp>
#include <Base/Window.hpp>
#include <Helper/ComPtr.hpp>
#include <Dx12/DescriptorHeap.hpp>
#include <DirectXTex/d3dx12.h>
#include <Helper/ThrowAssert.hpp>

class SceneManager;
struct SceneDesc
{
	std::shared_ptr<KGL::App> app;
	std::shared_ptr<KGL::Window> window;
};

class SceneBase
{
public:
	struct SceneBuffers
	{
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX proj;
		DirectX::XMFLOAT3 eye;	// éãì_ç¿ïW
	};
private:
	bool m_move_allow;
	bool m_loaded;
	std::mutex m_loaded_mutex;
	bool m_load_started;
	std::shared_ptr<SceneBase> m_next_scene;
private:
	HRESULT BaseLoad(const SceneDesc& desc);
public:
	SceneBase() : m_loaded(false), m_load_started(false), m_move_allow(true) {}
	virtual ~SceneBase() = default;
	HRESULT virtual Load(const SceneDesc& desc) = 0;
	HRESULT virtual Init(const SceneDesc& desc) { return S_OK; }
	HRESULT virtual Update(const SceneDesc& desc, float elapsed_time) = 0;
	HRESULT virtual UnInit(const SceneDesc& desc) { return S_OK; }

	bool IsLoaded() noexcept { std::lock_guard<std::mutex> lock(m_loaded_mutex); return m_loaded; };
	bool IsLoadStarted() const noexcept { return m_load_started; }
	template<class _Scene>
	void SetNextScene(const SceneDesc& desc, bool single_thread = false) noexcept
	{
		m_next_scene = std::make_shared<_Scene>();
		if (!single_thread)
		{
			m_load_started = true;
			std::thread(m_next_scene.get(), SceneBase::BaseLoad, desc).detach();
		}
	}
	void SetMoveSceneFlg(bool allow) noexcept { m_move_allow = allow; };
	bool IsAllowMoveScene() const noexcept { return m_move_allow; }
	const std::shared_ptr<SceneBase>& GetNextScene() noexcept { return m_next_scene; }
};

template <class _Ty>
class SceneBaseDx12 : public SceneBase
{
protected:
	std::shared_ptr<KGL::DescriptorManager>	scene_desc_mgr;
	KGL::DescriptorHandle					scene_buff_handle;
	KGL::ComPtr<ID3D12Resource>				scene_buff;
	_Ty*									scene_mapped_buff;
public:
	SceneBaseDx12() : scene_mapped_buff(nullptr) {}
	virtual ~SceneBaseDx12() = default;
	HRESULT virtual Load(const SceneDesc& desc) override;
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
	HRESULT UnInit(const SceneDesc& desc)
	{
		if (m_scene) return m_scene->UnInit(desc);
		return S_OK;
	}
	HRESULT Update(const SceneDesc& desc, float elapsed_time);
	HRESULT SceneChangeUpdate(const SceneDesc& desc);
};

template <class _Ty>
HRESULT SceneBaseDx12<_Ty>::Load(const SceneDesc& desc)
{
	HRESULT hr = S_OK;
	const auto& device = desc.app->GetDevice();

	scene_desc_mgr = std::make_shared<KGL::DescriptorManager>(device, 1u);
	scene_buff_handle = scene_desc_mgr->Alloc();

	const auto buff_size = (sizeof(_Ty) + 0xff) & ~0xff;
	hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(buff_size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(scene_buff.ReleaseAndGetAddressOf())
	);
	RCHECK(FAILED(hr), "CreateCommittedResourceÇ…é∏îs", hr);
	scene_mapped_buff = nullptr;
	hr = scene_buff->Map(0, nullptr, (void**)&scene_mapped_buff);
	RCHECK(FAILED(hr), "blur_const_buff->MapÇ…é∏îs", hr);

	D3D12_CONSTANT_BUFFER_VIEW_DESC mat_cbv_desc = {};
	mat_cbv_desc.BufferLocation = scene_buff->GetGPUVirtualAddress();
	mat_cbv_desc.SizeInBytes = buff_size;
	device->CreateConstantBufferView(&mat_cbv_desc, scene_buff_handle.Cpu());

	return hr;
}
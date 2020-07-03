#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <memory>
#include <mutex>

#include <Dx12/Application.hpp>
#include <Base/Window.hpp>

class SceneManager;
struct SceneDesc
{
	std::shared_ptr<KGL::App> app;
	std::shared_ptr<KGL::Window> window;
	KGL::ComPtr<ID3D12GraphicsCommandList> cmd_list;
};

class SceneBase
{
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
	HRESULT virtual Update(const SceneDesc& desc) = 0;
	HRESULT virtual Render(const SceneDesc& desc) = 0;
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
	HRESULT Update(const SceneDesc& desc);
	HRESULT SceneChangeUpdate(const SceneDesc& desc);
};
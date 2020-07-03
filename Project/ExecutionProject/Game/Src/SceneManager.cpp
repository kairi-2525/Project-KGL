#include "../Hrd/Scene.hpp"

HRESULT SceneBase::BaseLoad(const SceneDesc& desc)
{
	HRESULT hr = Load(desc);
	std::lock_guard<std::mutex> lock(m_loaded_mutex);
	m_loaded = true;
	return hr;
}

HRESULT SceneManager::Update(const SceneDesc& desc, float elapsed_time)
{
	return m_scene->Update(desc, elapsed_time);
}

HRESULT SceneManager::Render(const SceneDesc& desc, const KGL::ComPtr<ID3D12GraphicsCommandList>& cmd_list)
{
	return m_scene->Render(desc, cmd_list);
}

HRESULT SceneManager::SceneChangeUpdate(const SceneDesc& desc)
{
	HRESULT hr = S_OK;
	auto next_scene = m_scene->GetNextScene();
	if (next_scene)
	{
		if (!next_scene->IsLoadStarted())
		{
			hr = next_scene->Load(desc);
			if (FAILED(hr)) return hr;
		}
		else if (!next_scene->IsLoaded() || !m_scene->IsAllowMoveScene())
			return hr;

		m_scene->UnInit(desc);
		m_scene.reset();
		m_scene = next_scene;
		m_scene->Init(desc);
	}
	return hr;
}
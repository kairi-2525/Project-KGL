#include "../Hrd/Scene.hpp"
#include <DirectXTex/d3dx12.h>
#include <Helper/ThrowAssert.hpp>

HRESULT SceneBase::BaseLoad(const SceneDesc& desc)
{
	HRESULT hr = Load(desc);
	std::lock_guard<std::mutex> lock(m_loaded_mutex);
	m_loaded = true;
	return hr;
}

HRESULT SceneBaseDx12::Load(const SceneDesc& desc)
{
	HRESULT hr = S_OK;
	const auto& device = desc.app->GetDevice();

	scene_desc_mgr = std::make_shared<KGL::DescriptorManager>(device, 1u);
	scene_buff_handle = scene_desc_mgr->Alloc();

	const auto buff_size = (sizeof(SceneBuffers) + 0xff) & ~0xff;
	hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(buff_size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(scene_buff.ReleaseAndGetAddressOf())
	);
	RCHECK(FAILED(hr), "CreateCommittedResource‚ÉŽ¸”s", hr);
	mapped_scene_buff = nullptr;
	hr = scene_buff->Map(0, nullptr, (void**)&mapped_scene_buff);
	RCHECK(FAILED(hr), "blur_const_buff->Map‚ÉŽ¸”s", hr);

	D3D12_CONSTANT_BUFFER_VIEW_DESC mat_cbv_desc = {};
	mat_cbv_desc.BufferLocation = scene_buff->GetGPUVirtualAddress();
	mat_cbv_desc.SizeInBytes = buff_size;
	device->CreateConstantBufferView(&mat_cbv_desc, scene_buff_handle.Cpu());

	return hr;
}

HRESULT SceneManager::Update(const SceneDesc& desc, float elapsed_time)
{
	return m_scene->Update(desc, elapsed_time);
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

		desc.app->GetQueue()->Signal();
		desc.app->GetQueue()->Wait();

		m_scene->UnInit(desc);
		m_scene.reset();
		m_scene = next_scene;
		m_scene->Init(desc);
	}
	return hr;
}
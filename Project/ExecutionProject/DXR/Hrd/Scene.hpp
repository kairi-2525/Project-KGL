#pragma once

#include <Helper/Scene.hpp>
#include <Base/Window.hpp>
#include <Dx12/Application.hpp>
#include <Base/Input.hpp>

struct SceneDesc
{
	std::shared_ptr<KGL::Window>				window;
	std::shared_ptr<KGL::Application>			app;
	std::shared_ptr<KGL::Input>					input;
	std::shared_ptr<KGL::DescriptorManager>		imgui_heap;
	KGL::DescriptorHandle						imgui_handle;

};

using SceneBase = KGL::SceneBase<SceneDesc>;
using SceneManager = KGL::SceneManager<SceneDesc>;

class SceneMain : public SceneBase
{
private:
	KGL::ComPtr<ID3D12CommandAllocator>		cmd_allocator;
	KGL::ComPtr<ID3D12GraphicsCommandList4>	cmd_list4;
	KGL::ComPtr<ID3D12Device5>				device5;
public:
	HRESULT Load(const SceneDesc& desc) override;
	HRESULT Init(const SceneDesc& desc) override;
	HRESULT Update(const SceneDesc& desc, float elapsed_time) override;
	HRESULT Render(const SceneDesc& desc);
	HRESULT UnInit(const SceneDesc& desc, std::shared_ptr<SceneBase> next_scene) override;
};
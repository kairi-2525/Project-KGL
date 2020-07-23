#pragma once

#include "../Scene.hpp"
#include <Dx12/Texture.hpp>
#include <Loader/PMDLoader.hpp>
#include <Loader/VMDLoader.hpp>
#include <Dx12/2D/Sprite.hpp>
#include <Dx12/3D/PMDModel.hpp>
#include <Dx12/3D/PMDRenderer.hpp>
#include <Dx12/2D/Renderer.hpp>
#include <Dx12/PipelineState.hpp>
#include <Dx12/Shader.hpp>
#include <Base/Camera.hpp>
#include <Dx12/RenderTargetView.hpp>
#include <Dx12/DescriptorHeap.hpp>

#include "../Obj3D.hpp"

class TestScene02 : public SceneBase
{
private:
	static inline const UINT SHADOW_DIFINITION = 2048u;
private:
	SceneBufferDx12<SceneBuffers>			scene_buffer;

	KGL::VecCamera camera;
	KGL::Camera light_camera;
	
	std::shared_ptr<KGL::RenderTargetView>	rtv;
	std::shared_ptr<KGL::DescriptorManager>	light_dsv_desc_mgr;
	KGL::DescriptorHandle					light_dsv_handle;
	std::shared_ptr<KGL::Texture>			light_dsv;

	std::shared_ptr<KGL::BaseRenderer>		pmd_light_renderer;
	std::shared_ptr<KGL::BaseRenderer>		depth_renderer;
	std::shared_ptr<KGL::Sprite>			sprite;

	KGL::TextureManager						tex_mgr;
	std::shared_ptr<KGL::PMD_Loader>		pmd_data;
	std::shared_ptr<KGL::VMD_Loader>		vmd_data;
	std::shared_ptr<KGL::PMD_Model>			pmd_model;
	std::shared_ptr<KGL::PMD_Model>			pmd_toon_model;
	std::shared_ptr<KGL::PMD_Renderer>		pmd_renderer;

	std::vector<Obj3D>						models;

	KGL::ComPtr<ID3D12CommandAllocator>		cmd_allocator;
	KGL::ComPtr<ID3D12GraphicsCommandList>	cmd_list;

	DirectX::XMFLOAT4						clear_color;

	std::shared_ptr<KGL::DescriptorManager>	dsv_srv_desc_mgr;
	KGL::DescriptorHandle					dsv_srv_handle;

public:
	HRESULT Load(const SceneDesc& desc) override;
	HRESULT Init(const SceneDesc& desc) override;
	HRESULT Update(const SceneDesc& desc, float elapsed_time) override;
	HRESULT Render(const SceneDesc& desc);
	HRESULT UnInit(const SceneDesc& desc, std::shared_ptr<SceneBase> next_scene) override;

	TestScene02() = default;
	~TestScene02() = default;
};
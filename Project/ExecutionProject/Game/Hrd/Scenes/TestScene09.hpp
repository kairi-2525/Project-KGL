#pragma once

#include "../Scene.hpp"
#include <Dx12/Texture.hpp>
#include <Dx12/3D/Renderer.hpp>
#include <Dx12/2D/Renderer.hpp>
#include <Dx12/2D/Sprite.hpp>
#include <Dx12/PipelineState.hpp>
#include <Dx12/Shader.hpp>
#include "../Camera.hpp"
#include <Dx12/RenderTargetView.hpp>
#include <Dx12/DescriptorHeap.hpp>
#include <Dx12/ConstantBuffer.hpp>
#include <Dx12/Compute.hpp>
#include <Dx12/3D/Board.hpp>

#include "Global.hpp"
#include "../Obj3D.hpp"
#include <Dx12/3D/StaticModelActor.hpp>

class TestScene09 : public SceneBase
{
private:
	enum RT
	{
		WORLD,
		WORLD_BT,	// WORLD‚ðBrighten‚µ‚Ä–¾‚é‚­‚µ‚½Œã‚ÌRT
		FXAA,
	};
public:
	struct CubeMapBuffer
	{
		DirectX::XMFLOAT4X4	view_mat[6];
		DirectX::XMFLOAT4X4 proj;
	};
private:
	KGL::ComPtr<ID3D12CommandAllocator>					cmd_allocator;
	KGL::ComPtr<ID3D12GraphicsCommandList>				cmd_list;
	std::shared_ptr<KGL::RenderTargetView>				rtvs;
	std::vector<std::shared_ptr<KGL::Texture>>			rt_textures;
	std::shared_ptr<KGL::DescriptorManager>				descriptor;

	std::shared_ptr<KGL::Resource<FrameBuffer>>			frame_buffer;
	std::shared_ptr<KGL::DescriptorHandle>				frame_buffer_handle;
	std::shared_ptr<FPSCamera>							camera;

	std::shared_ptr<KGL::RenderTargetView>				cube_rtv;
	std::shared_ptr<KGL::Resource<CubeMapBuffer>>		cube_buffer;
	std::shared_ptr<KGL::DescriptorHandle>				cube_buffer_handle;
	std::shared_ptr<KGL::DescriptorManager>				cube_dsv_descriptor;
	std::shared_ptr<KGL::DescriptorHandle>				cube_dsv_handle;

	std::vector<std::shared_ptr<KGL::StaticModelActor>>	s_actors;
	std::shared_ptr<KGL::StaticModelActor>				inc_actor;
	std::shared_ptr<KGL::StaticModelActor>				slime_actor;
	std::shared_ptr<KGL::StaticModelActor>				sky_actor;
	std::shared_ptr<KGL::StaticModelActor>				earth_actor;
	std::shared_ptr<KGL::StaticModelActor>				bison_actor;
	std::shared_ptr<KGL::BaseRenderer>					s_model_renderer;
	std::shared_ptr<KGL::BaseRenderer>					sky_renderer;

	std::shared_ptr<KGL::TextureCube>					cube_texture;
	std::shared_ptr<KGL::TextureCube>					cube_depth_texture;

	std::shared_ptr<FXAAManager>						fxaa_mgr;
	std::shared_ptr<KGL::BaseRenderer>					sprite_renderer;
	std::shared_ptr<KGL::BaseRenderer>					brighten_renderer;
	std::shared_ptr<KGL::Sprite>						sprite;
public:
	HRESULT Load(const SceneDesc& desc) override;
	HRESULT Init(const SceneDesc& desc) override;
	HRESULT Update(const SceneDesc& desc, float elapsed_time) override;
	HRESULT RenderCubeMap(const SceneDesc& desc);
	HRESULT Render(const SceneDesc& desc);
	HRESULT UnInit(const SceneDesc& desc, std::shared_ptr<SceneBase> next_scene) override;

	TestScene09() = default;
	~TestScene09() = default;
};
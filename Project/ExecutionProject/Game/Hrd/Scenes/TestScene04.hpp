#pragma once

#include "../Scene.hpp"
#include <Dx12/Texture.hpp>
#include <Loader/PMDLoader.hpp>
#include <Loader/VMDLoader.hpp>
#include <Dx12/3D/PMDModel.hpp>
#include <Dx12/3D/PMDRenderer.hpp>
#include <Dx12/2D/Renderer.hpp>
#include <Dx12/2D/Sprite.hpp>
#include <Dx12/PipelineState.hpp>
#include <Dx12/Shader.hpp>
#include <Base/Camera.hpp>
#include <Dx12/RenderTargetView.hpp>
#include <Dx12/DescriptorHeap.hpp>
#include <Dx12/ConstantBuffer.hpp>
#include <Dx12/Compute.hpp>

#include "../Obj3D.hpp"

class TestScene04 : public SceneBase
{
	struct Particle
	{
		DirectX::XMFLOAT3 position; float pad0;
		DirectX::XMFLOAT3 scale; float pad1;
		DirectX::XMFLOAT3 velocity; float pad2;
		DirectX::XMFLOAT3 accs; float pad3;
		bool exist;
	};
	struct SceneBuffers
	{
		DirectX::XMFLOAT4X4	view_proj;
		DirectX::XMFLOAT4X4	re_view;
		float				elapsed_time;
	};
private:
	DirectX::XMFLOAT4X4							proj;

	SceneBufferDx12<SceneBuffers>				scene_buffer;
	KGL::VecCamera								camera;

	std::shared_ptr<KGL::BaseRenderer>			sprite_renderer;
	std::shared_ptr<KGL::Sprite>				sprite;

	KGL::TextureManager							tex_mgr;

	KGL::ComPtr<ID3D12CommandAllocator>			cmd_allocator;
	KGL::ComPtr<ID3D12GraphicsCommandList>		cmd_list;

	KGL::ComPtr<ID3D12CommandAllocator>			cpt_cmd_allocator;
	KGL::ComPtr<ID3D12GraphicsCommandList>		cpt_cmd_list;
	std::shared_ptr<KGL::CommandQueue>			cpt_cmd_queue;

	std::shared_ptr<KGL::RenderTargetView>		rtvs;
	std::vector<std::shared_ptr<KGL::Texture>>	rtv_textures;

	std::shared_ptr<KGL::Resource<Particle>>	particle_resource;
	std::shared_ptr<KGL::DescriptorManager>		particle_desc_mgr;
	std::shared_ptr<KGL::ComputePipline>		particle_pipeline;
	KGL::DescriptorHandle						particle_begin_handle;
public:
	HRESULT Load(const SceneDesc& desc) override;
	HRESULT Init(const SceneDesc& desc) override;
	HRESULT Update(const SceneDesc& desc, float elapsed_time) override;
	HRESULT Render(const SceneDesc& desc);
	HRESULT UnInit(const SceneDesc& desc) override;

	TestScene04() = default;
	~TestScene04() = default;
};
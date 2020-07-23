#pragma once

#include "../Scene.hpp"
#include <Dx12/Texture.hpp>
#include <Loader/PMDLoader.hpp>
#include <Loader/VMDLoader.hpp>
#include <Dx12/3D/PMDModel.hpp>
#include <Dx12/3D/PMDRenderer.hpp>
#include <Dx12/3D/Renderer.hpp>
#include <Dx12/2D/Renderer.hpp>
#include <Dx12/2D/Sprite.hpp>
#include <Dx12/PipelineState.hpp>
#include <Dx12/Shader.hpp>
#include <Base/Camera.hpp>
#include <Dx12/RenderTargetView.hpp>
#include <Dx12/DescriptorHeap.hpp>
#include <Dx12/ConstantBuffer.hpp>
#include <Dx12/Compute.hpp>
#include <Dx12/3D/Board.hpp>

#include "../Obj3D.hpp"

class TestScene05 : public SceneBase
{
	struct PTC
	{
		DirectX::XMFLOAT4 a;
		DirectX::XMFLOAT4 b;
		DirectX::XMFLOAT4 c;
		DirectX::XMFLOAT3 d;
		float e;
	};
private:
	std::shared_ptr<KGL::Resource<PTC>>		cpt_resource;

	KGL::ComPtr<ID3D12CommandAllocator>			cpt_cmd_allocator;
	KGL::ComPtr<ID3D12GraphicsCommandList>		cpt_cmd_list;
	std::shared_ptr<KGL::CommandQueue>			cpt_cmd_queue;
	std::shared_ptr<KGL::DescriptorManager>		cpt_descmgr;
	KGL::DescriptorHandle						cpt_handle;
	std::shared_ptr<KGL::ComputePipline>		cpt_pipeline;

public:
	HRESULT Load(const SceneDesc& desc) override;
	HRESULT Init(const SceneDesc& desc) override;
	HRESULT Update(const SceneDesc& desc, float elapsed_time) override;
	HRESULT Render(const SceneDesc& desc);
	HRESULT UnInit(const SceneDesc& desc, std::shared_ptr<SceneBase> next_scene) override;

	TestScene05() = default;
	~TestScene05() = default;
};
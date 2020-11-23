#pragma once

#include "../Scene.hpp"
#include <Dx12/Texture.hpp>
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

class TestScene07 : public SceneBase
{
private:
	struct StepBuffer
	{
		UINT32	block_step;
		UINT32	sub_block_step;
	};
	using SpDpHandle = std::shared_ptr<KGL::DescriptorHandle>;
private:
	UINT32										lgn;
	UINT32										step_max;
	UINT										rs_idx;
	std::shared_ptr<KGL::Resource<UINT32>>		value_rs[2];
	std::shared_ptr<KGL::Resource<UINT32>>		frame_buff_rs;
	std::shared_ptr<KGL::MultiResource<StepBuffer>>	step_buff_rs;

	KGL::ComPtr<ID3D12CommandAllocator>			cpt_cmd_allocator;
	KGL::ComPtr<ID3D12GraphicsCommandList>		cpt_cmd_list;
	std::shared_ptr<KGL::CommandQueue>			cpt_cmd_queue;
	std::shared_ptr<KGL::DescriptorManager>		cpt_descmgr;
	SpDpHandle									value_uav_handle[2];
	SpDpHandle									frame_cbv_handle;
	std::vector<SpDpHandle>						step_cbv_handles;
	std::shared_ptr<KGL::ComputePipline>		cpt_pipeline;

public:
	HRESULT Load(const SceneDesc& desc) override;
	HRESULT Init(const SceneDesc& desc) override;
	HRESULT Update(const SceneDesc& desc, float elapsed_time) override;
	HRESULT Render(const SceneDesc& desc);
	HRESULT UnInit(const SceneDesc& desc, std::shared_ptr<SceneBase> next_scene) override;

	void OutputValue();
	void CountPlus();

	TestScene07() = default;
	~TestScene07() = default;
};
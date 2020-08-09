#pragma once

#include <Helper/Scene.hpp>
#include <Base/Window.hpp>
#include <Dx12/Application.hpp>
#include <Base/Input.hpp>
#include <Dx12/ConstantBuffer.hpp>
#include <Dx12/2D/Renderer.hpp>
#include "../DXRHelper/nv_helpers_dx12/TopLevelASGenerator.h"

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
	KGL::ComPtr<ID3D12CommandAllocator>					cmd_allocator;
	KGL::ComPtr<ID3D12GraphicsCommandList4>				cmd_list4;
	KGL::ComPtr<ID3D12Device5>							device5;

	bool												raster;

public:
	struct TriangleVertex
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT4 color;
	};
	std::shared_ptr<KGL::Resource<TriangleVertex>>		t_vert_res;
	D3D12_VERTEX_BUFFER_VIEW							t_vert_view;
	std::shared_ptr<KGL::BaseRenderer>					t_renderer;

	struct AccelerationStructureBuffers
	{
		KGL::ComPtr<ID3D12Resource>		scratch;		// ASビルダーのスクラッチメモリ
		KGL::ComPtr<ID3D12Resource>		result;			// ASの場所
		KGL::ComPtr<ID3D12Resource>		instance_desc;	// インスタンスの行列を保持する
	};
	KGL::ComPtr<ID3D12Resource>							bottom_level_as;		// 最下位レベルのASのストレージ
	nv_helpers_dx12::TopLevelASGenerator				top_level_as_generator;
	AccelerationStructureBuffers						top_level_buffers;
	std::vector<std::pair<KGL::ComPtr<ID3D12Resource>, DirectX::XMMATRIX>> instances;

public:
	AccelerationStructureBuffers CreateBottomLevelAS(
		const std::vector<std::pair<KGL::ComPtr<ID3D12Resource>, uint32_t>>& vertex_buffers);
	void CreateTopLevelAS(const std::vector<std::pair<KGL::ComPtr<ID3D12Resource>, DirectX::XMMATRIX>> &instances);
	void CreateAccelerationStructures(const std::shared_ptr<KGL::CommandQueue>& queue);
public:
	HRESULT Load(const SceneDesc& desc) override;
	HRESULT Init(const SceneDesc& desc) override;
	HRESULT Update(const SceneDesc& desc, float elapsed_time) override;
	HRESULT Render(const SceneDesc& desc);
	HRESULT UnInit(const SceneDesc& desc, std::shared_ptr<SceneBase> next_scene) override;
};
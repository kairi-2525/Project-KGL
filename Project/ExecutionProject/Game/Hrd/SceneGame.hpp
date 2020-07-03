#pragma once

#include "Scene.hpp"
#include <Dx12/Texture.hpp>
#include <Loader/PMDLoader.hpp>
#include <Dx12/PMDModel.hpp>
#include <Dx12/PipelineState.hpp>
#include <Dx12/Shader.hpp>
#include <Base/Camera.hpp>

class SceneGame : public SceneBase
{
private:
	struct SceneMatrix
	{
		DirectX::XMMATRIX wvp;
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX proj;
		DirectX::XMFLOAT3 eye;	// éãì_ç¿ïW
	};
private:
	SceneMatrix* map_buffer;
	KGL::VecCamera camera;

	std::shared_ptr<KGL::Texture>		texture;
	std::shared_ptr<KGL::PMD_Loader>	pmd_data;
	std::shared_ptr<KGL::PMD_Model>		pmd_model;

	std::shared_ptr<KGL::Shader>		shader;
	KGL::ComPtr<ID3D12PipelineState>	pl_state;
	KGL::ComPtr<ID3D12RootSignature>	rootsig;

	KGL::ComPtr<ID3D12DescriptorHeap>	basic_desc_heap;
	KGL::ComPtr<ID3D12Resource>			const_buff;

	KGL::ComPtr<ID3D12DescriptorHeap> material_heap;
public:
	HRESULT Load(const SceneDesc& desc) override;
	HRESULT Init(const SceneDesc& desc) override;
	HRESULT Update(const SceneDesc& desc) override;
	HRESULT Render(const SceneDesc& desc) override;
	HRESULT UnInit(const SceneDesc& desc) override;

	SceneGame() = default;
	~SceneGame() = default;
};
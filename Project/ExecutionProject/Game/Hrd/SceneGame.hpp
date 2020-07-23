#pragma once

#include "Scene.hpp"
#include <Dx12/Texture.hpp>
#include <Loader/PMDLoader.hpp>
#include <Loader/VMDLoader.hpp>
#include <Dx12/3D/PMDModel.hpp>
#include <Dx12/3D/PMDRenderer.hpp>
#include <Dx12/PipelineState.hpp>
#include <Dx12/Shader.hpp>
#include <Base/Camera.hpp>

#include "Obj3D.hpp"

class SceneGame : public SceneBase
{
private:
	struct SceneMatrix
	{
		DirectX::XMMATRIX wvp;
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX proj;
		DirectX::XMMATRIX bones[512];
		DirectX::XMFLOAT3 eye;	// éãì_ç¿ïW
	};
private:
	KGL::VecCamera camera;

	KGL::TextureManager						tex_mgr;
	std::shared_ptr<KGL::Texture>			texture;
	std::shared_ptr<KGL::PMD_Loader>		pmd_data;
	std::shared_ptr<KGL::VMD_Loader>		vmd_data;
	std::shared_ptr<KGL::PMD_Model>			pmd_model;
	std::shared_ptr<KGL::PMD_Model>			pmd_toon_model;
	std::shared_ptr<KGL::PMD_Renderer>		pmd_renderer;

	std::vector<Obj3D>						models;

	KGL::ComPtr<ID3D12CommandAllocator>		cmd_allocator;
	KGL::ComPtr<ID3D12GraphicsCommandList>	cmd_list;

	DirectX::XMFLOAT4						clear_color;

public:
	HRESULT Load(const SceneDesc& desc) override;
	HRESULT Init(const SceneDesc& desc) override;
	HRESULT Update(const SceneDesc& desc, float elapsed_time) override;
	HRESULT Render(const SceneDesc& desc);
	HRESULT UnInit(const SceneDesc& desc, std::shared_ptr<SceneBase> next_scene) override;

	SceneGame() = default;
	~SceneGame() = default;
};
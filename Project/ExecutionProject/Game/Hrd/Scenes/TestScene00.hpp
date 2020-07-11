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

class TestScene00 : public SceneBase
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

	std::shared_ptr<KGL::PMD_Loader>		pmd_data;
	std::shared_ptr<KGL::VMD_Loader>		vmd_data;
	std::shared_ptr<KGL::PMD_Model>			pmd_model;
	std::shared_ptr<KGL::PMD_Model>			pmd_toon_model;
	std::shared_ptr<KGL::PMD_Renderer>		pmd_renderer;

	std::shared_ptr<KGL::RenderTargetView>	texture_rtv;
	KGL::TextureManager						tex_mgr;
	std::shared_ptr<KGL::Texture>			tex_blur_w;
	std::shared_ptr<KGL::Texture>			tex_blur_h;
	std::shared_ptr<KGL::BaseRenderer>		renderer_sprite;
	std::shared_ptr<KGL::BaseRenderer>		renderer_blur_w;
	std::shared_ptr<KGL::BaseRenderer>		renderer_blur_h;
	std::shared_ptr<KGL::Sprite>			sprite;

	std::shared_ptr<KGL::Texture>			tex_rendet_target;
	std::shared_ptr<KGL::DescriptorManager>	effect_desc_mgr;
	KGL::DescriptorHandle					effect_desc_handle;
	std::shared_ptr<KGL::Texture>			tex_effect;
	std::shared_ptr<KGL::BaseRenderer>		renderer_effect;

	std::vector<Obj3D>						models;

	KGL::ComPtr<ID3D12CommandAllocator>		cmd_allocator;
	KGL::ComPtr<ID3D12GraphicsCommandList>	cmd_list;

	DirectX::XMFLOAT4						clear_color;

	std::shared_ptr<KGL::DescriptorManager>	blur_desc_mgr;
	KGL::ComPtr<ID3D12Resource>				blur_const_buff;
	KGL::DescriptorHandle					blur_buff_handle;
public:
	HRESULT Load(const SceneDesc& desc) override;
	HRESULT Init(const SceneDesc& desc) override;
	HRESULT Update(const SceneDesc& desc, float elapsed_time) override;
	HRESULT Render(const SceneDesc& desc);
	HRESULT UnInit(const SceneDesc& desc) override;

	TestScene00() = default;
	~TestScene00() = default;
};
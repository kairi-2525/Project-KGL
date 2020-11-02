#pragma once

#include <DirectXMath.h>
#include <Dx12/Texture.hpp>
#include <Dx12/RenderTargetView.hpp>
#include <Dx12/2D/Renderer.hpp>
#include <Dx12/2D/Sprite.hpp>
#include <Dx12/ConstantBuffer.hpp>
#include <Dx12/DescriptorHeap.hpp>

class DOFGenerator
{
public:
	static inline const UINT8							RTV_MAX = 8u;
private:
	struct RTVResources
	{
		std::array<std::shared_ptr<KGL::Texture>, RTV_MAX> textures;
		std::shared_ptr<KGL::RenderTargetView> rtvs;
	};
private:
	std::shared_ptr<KGL::BaseRenderer>					dof_renderer;
	std::shared_ptr<KGL::BaseRenderer>					compression_renderer;
	std::shared_ptr<KGL::BaseRenderer>					gaussian_renderer_w;
	std::shared_ptr<KGL::BaseRenderer>					gaussian_renderer_h;
	RTVResources										compression_rtvr;
	RTVResources										gaussian_rtvr_w;
	RTVResources										gaussian_rtvr_h;
	std::shared_ptr<KGL::Texture>						before_texture;

	std::shared_ptr<KGL::Sprite>						sprite;
	std::shared_ptr<KGL::Resource<UINT32>>				rtv_num_res;
	std::shared_ptr<KGL::DescriptorManager>				srv_cbv_dsmgr;
	KGL::DescriptorHandle								rtv_num_handle;
	std::shared_ptr<KGL::Resource<float>>				gaussian_buffer;
	KGL::DescriptorHandle								gausian_buffer_handle;
	std::shared_ptr<KGL::DescriptorHandle>				before_tex_srv_handle;
public:
	DOFGenerator(KGL::ComPtrC<ID3D12Device> device,
		const std::shared_ptr<KGL::DXC>& dxc, KGL::ComPtrC<ID3D12Resource> rsc);
	void Generate(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list,
		std::shared_ptr<KGL::Texture> texture,
		KGL::ComPtrC<ID3D12DescriptorHeap> srv_heap, D3D12_GPU_DESCRIPTOR_HANDLE srv_gpu_handle,
		D3D12_VIEWPORT view_port);
	void Render(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list,
		KGL::ComPtrC<ID3D12DescriptorHeap> depth_heap, D3D12_GPU_DESCRIPTOR_HANDLE depth_srv_handle);
	const std::array<std::shared_ptr<KGL::Texture>, 8u>& GetTextures() const noexcept { return gaussian_rtvr_h.textures; }
	void SetRtvNum(UINT8 num);
	UINT8 GetRtvNum();
};
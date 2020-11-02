#pragma once

#include <DirectXMath.h>
#include <Dx12/Texture.hpp>
#include <Dx12/RenderTargetView.hpp>
#include <Dx12/2D/Renderer.hpp>
#include <Dx12/2D/Sprite.hpp>
#include <Dx12/ConstantBuffer.hpp>
#include <Dx12/DescriptorHeap.hpp>

class BloomGenerator
{
public:
	static inline const UINT8							RTV_MAX = 8u;
	using Weights = std::array<float, RTV_MAX>;
	
	struct Buffer
	{
		UINT32	kernel;
		UINT32	gaussian_kernel;
		Weights	weight;
	};

	struct RTVResources
	{
		std::array<std::shared_ptr<KGL::Texture>, RTV_MAX> textures;
		std::shared_ptr<KGL::RenderTargetView> rtvs;
	};
private:
	std::shared_ptr<KGL::BaseRenderer>					bloom_renderer;
	std::shared_ptr<KGL::BaseRenderer>					compression_renderer;
	std::shared_ptr<KGL::BaseRenderer>					gaussian_renderer_w;
	std::shared_ptr<KGL::BaseRenderer>					gaussian_renderer_h;
	RTVResources										compression_rtvr;
	RTVResources										gaussian_rtvr_w;
	RTVResources										gaussian_rtvr_h;
	std::shared_ptr<KGL::RenderTargetView>				bloom_rtv;
	std::shared_ptr<KGL::Texture>						bloom_texture;

	std::shared_ptr<KGL::Sprite>						sprite;
	std::shared_ptr<KGL::Resource<Buffer>>				frame_buffer;
	std::shared_ptr<KGL::Resource<float>>				gaussian_buffer;
	std::shared_ptr<KGL::DescriptorManager>				constant_buffer_dsmgr;
	KGL::DescriptorHandle								frame_buffer_handle;
	KGL::DescriptorHandle								gausian_buffer_handle;
public:
	BloomGenerator(KGL::ComPtrC<ID3D12Device> device, 
		const std::shared_ptr<KGL::DXC>& dxc, KGL::ComPtrC<ID3D12Resource> rsc);
	void Generate(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list,
		const KGL::DescriptorHandle& srv_handle, D3D12_VIEWPORT view_port)
	{
		return Generate(cmd_list, srv_handle.Heap(), srv_handle.Gpu(), view_port);
	}
	void Generate(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list,
		KGL::ComPtrC<ID3D12DescriptorHeap> srv_heap, D3D12_GPU_DESCRIPTOR_HANDLE srv_gpu_handle,
		D3D12_VIEWPORT view_port);
	void Render(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list);
	const std::array<std::shared_ptr<KGL::Texture>, RTV_MAX>& GetTexturesC() const noexcept { return compression_rtvr.textures; }
	const std::array<std::shared_ptr<KGL::Texture>, RTV_MAX>& GetTexturesW() const noexcept { return gaussian_rtvr_w.textures; }
	const std::array<std::shared_ptr<KGL::Texture>, RTV_MAX>& GetTexturesH() const noexcept { return gaussian_rtvr_h.textures; }
	const std::shared_ptr<KGL::Texture>& GetTexture() const noexcept { return bloom_texture; }
	void SetKernel(UINT8 num) noexcept;
	UINT8 GetKernel() const noexcept;
	void SetGaussianKernel(UINT8 num) noexcept;
	UINT8 GetGaussianKernel() const noexcept;
	void SetWeights(Weights weights) noexcept;
	Weights GetWeights() const noexcept;
};
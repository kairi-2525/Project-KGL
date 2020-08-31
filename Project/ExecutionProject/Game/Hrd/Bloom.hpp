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
		Weights	weight;
	};
private:
	std::shared_ptr<KGL::BaseRenderer>					bloom_renderer;
	std::shared_ptr<KGL::BaseRenderer>					renderer;
	std::shared_ptr<KGL::RenderTargetView>				rtvs;
	std::array<std::shared_ptr<KGL::Texture>, RTV_MAX>	rtv_textures;
	std::shared_ptr<KGL::Sprite>						sprite;
	std::shared_ptr<KGL::Resource<Buffer>>				buffer_res;
	std::shared_ptr<KGL::DescriptorManager>				rtv_num_dsmgr;
	KGL::DescriptorHandle								rtv_num_handle;
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
	const std::array<std::shared_ptr<KGL::Texture>, 8u>& GetTextures() const noexcept { return rtv_textures; }
	void SetKernel(UINT8 num) noexcept;
	UINT8 GetKernel() const noexcept;
	void SetWeights(Weights weights) noexcept;
	Weights GetWeights() const noexcept;
};
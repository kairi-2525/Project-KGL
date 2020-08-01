#pragma once

#include <DirectXMath.h>
#include <Dx12/Texture.hpp>
#include <Dx12/RenderTargetView.hpp>
#include <Dx12/2D/Renderer.hpp>
#include <Dx12/2D/Sprite.hpp>

class BloomGenerator
{
	std::shared_ptr<KGL::BaseRenderer>			bloom_renderer;
	std::shared_ptr<KGL::BaseRenderer>			renderer;
	std::shared_ptr<KGL::RenderTargetView>		rtvs;
	std::shared_ptr<KGL::Texture>				rtv_texture;
	std::shared_ptr<KGL::Sprite>				sprite;
public:
	BloomGenerator(KGL::ComPtrC<ID3D12Device> device, KGL::ComPtrC<ID3D12Resource> rsc);
	void Generate(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list,
		const KGL::DescriptorHandle& srv_handle, D3D12_VIEWPORT view_port)
	{
		return Generate(cmd_list, srv_handle.Heap(), srv_handle.Gpu(), view_port);
	}
	void Generate(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list,
		KGL::ComPtrC<ID3D12DescriptorHeap> srv_heap, D3D12_GPU_DESCRIPTOR_HANDLE srv_gpu_handle,
		D3D12_VIEWPORT view_port);
	void Render(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list);
	const std::shared_ptr<KGL::Texture>& GetTexture() const noexcept { return rtv_texture; }
};
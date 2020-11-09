#pragma once

#include "../Scene.hpp"
#include <Dx12/Texture.hpp>
#include <Dx12/2D/Sprite.hpp>
#include <Dx12/2D/Renderer.hpp>
#include <Dx12/PipelineState.hpp>
#include <Dx12/ConstantBuffer.hpp>
#include <Dx12/Shader.hpp>
#include <Base/Camera.hpp>
#include <Dx12/RenderTargetView.hpp>
#include <Dx12/DescriptorHeap.hpp>

#include "../Obj3D.hpp"

class LoadScene00Base : public SceneBase
{
public:
	struct FrameBuffer
	{
		DirectX::XMFLOAT3	color;
		float				time;
		DirectX::XMFLOAT2	resolution;
		float				rotate_scale;
	};
private:
	KGL::VecCamera camera;
	KGL::TextureManager								tex_mgr;
	KGL::ComPtr<ID3D12CommandAllocator>				cmd_allocator;
	KGL::ComPtr<ID3D12GraphicsCommandList>			cmd_list;

	std::shared_ptr<KGL::_2D::Sprite>				sprite;
	std::shared_ptr<KGL::DescriptorManager>			cbv_srv_descriptor;
	std::shared_ptr<KGL::Resource<FrameBuffer>>		frame_buffer_resource;
	std::shared_ptr<KGL::DescriptorHandle>			frame_buffer_handle;
	std::shared_ptr<KGL::Texture>					noise_texture;
	std::shared_ptr<KGL::DescriptorHandle>			noise_srv_handle;
	std::shared_ptr<KGL::BaseRenderer>				noise_anim_renderer;

	DirectX::XMFLOAT4								clear_color;
	DirectX::XMFLOAT3								begin_color;
	DirectX::XMFLOAT3								end_color;
	float											counter;
	float											counter_max;
protected:
	virtual void SetLoadScene(const SceneDesc& desc) = 0;
public:
	HRESULT Load(const SceneDesc& desc) override;
	HRESULT Init(const SceneDesc& desc) override;
	HRESULT Update(const SceneDesc& desc, float elapsed_time) override;
	HRESULT Render(const SceneDesc& desc);
	HRESULT UnInit(const SceneDesc& desc, std::shared_ptr<SceneBase> next_scene) override;

	LoadScene00Base() = default;
	virtual ~LoadScene00Base() = default;
};

template <class _Ty>
class LoadScene00 : public LoadScene00Base
{
protected:
	void SetLoadScene(const SceneDesc& desc) override
	{
		SetNextScene<_Ty>(desc);
	}
};
#pragma once

#include <Dx12/2D/Renderer.hpp>
#include <Dx12/ConstantBuffer.hpp>
#include <Dx12/DescriptorHeap.hpp>
#include <DirectXMath.h>

using UINT = unsigned int;

class FXAAManager
{
public:
	enum TYPE : UINT
	{
		FXAA_OFF,
		FXAA_ON
	};
	struct CBuffer
	{
		float quality_edge_threshold;
		float quality_edge_threshold_min;
		float console_edge_threshold;
		float console_edge_threshold_min;
		float edge_sharpness;
		float subpix;
		DirectX::XMFLOAT2 rcp_frame;
		DirectX::XMFLOAT4 rcp_frame_opt;
		DirectX::XMFLOAT4 rcp_frame_opt2;
	};
	struct Desc
	{
		TYPE type;
		CBuffer buffer;
	};
	static const inline CBuffer DEFAULT_BUFFER
	{
		0.166f,
		0.0833f,
		0.125f,
		0.05f,
		8.0f,
		0.75f
		// RCP‚Í‰æ–ÊƒTƒCƒYˆË‘¶
	};
private:
	Desc desc;
	float n;
	bool changed;
	std::shared_ptr<KGL::BaseRenderer>		gray_renderer;
	std::shared_ptr<KGL::BaseRenderer>		renderer;
	std::shared_ptr<KGL::Resource<CBuffer>> gpu_resource;
	std::shared_ptr<KGL::DescriptorManager>	cbv_descriptor;
	KGL::DescriptorHandle					cbv_handle;
public:
	static DirectX::XMFLOAT2 GetRCPFrame(const DirectX::XMUINT2& rt_size);
	static DirectX::XMFLOAT4 GetRCPFrameOpt(const DirectX::XMUINT2& rt_size, float N = 0.50f);
	static DirectX::XMFLOAT4 GetRCPFrameOpt2(const DirectX::XMUINT2& rt_size) { return GetRCPFrameOpt(rt_size, 2.0f); }
public:
	FXAAManager(ComPtrC<ID3D12Device> device, const std::shared_ptr<KGL::DXC>& dxc, const DirectX::XMUINT2& rt_size);
	void SetDesc(Desc desc) { changed = true; this->desc = desc; }
	void UpdateBuffer();
	const Desc& GetDesc() const { return desc; }
	void SetRCPFrameDesc(const DirectX::XMUINT2& rt_size, float N = 0.50f);
	void ImGuiTreeUpdate(const DirectX::XMUINT2& rt_size);
	float GetN() const { return n; }
	void SetState(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list);
	void SetGrayState(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list);
	bool IsActive() const { return desc.type == TYPE::FXAA_ON; }
	void SetActiveFlg(bool flg) { desc.type = flg ? TYPE::FXAA_ON : TYPE::FXAA_OFF; }
};
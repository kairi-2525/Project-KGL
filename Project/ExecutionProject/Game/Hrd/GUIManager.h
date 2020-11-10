#pragma once

#include <DirectXMath.h>
#include <Dx12/Texture.hpp>
#include <Dx12/DescriptorHeap.hpp>
#include <imgui.h>

class GUIManager
{
private:
	ImGuiWindowFlags	main_window_flag;
	std::shared_ptr<KGL::DescriptorManager>	imgui_srv_desc;
	std::vector<std::shared_ptr<KGL::DescriptorHandle>> imgui_srv_handles;

	std::vector<std::shared_ptr<KGL::Texture>> textures;
public:
	GUIManager(ComPtrC<ID3D12Device> device, std::shared_ptr<KGL::DescriptorManager> imgui_descriptor);
	~GUIManager();
	void Init();
	float time_scale;	// ŽžŠÔ”{‘¬
	void Update(const DirectX::XMUINT2& rt_resolution);
};
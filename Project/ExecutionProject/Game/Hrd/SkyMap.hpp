#pragma once

#include <DirectXMath.h>
#include <array>
#include <memory>
#include <Dx12/DescriptorHeap.hpp>
#include <Dx12/Texture.hpp>
#include <Dx12/ConstantBuffer.hpp>
#include <Dx12/Base/Renderer.hpp>

class SkyManager
{
public:
	enum CUBE
	{
		FRONT,
		BACK,
		RIGHT,
		LEFT,
		TOP,
		BOTTOM,
		NUM
	};
	struct Vertex
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT2 uv;
	};
	struct Tex
	{
		std::array<KGL::DescriptorHandle, CUBE::NUM>			handle;
		std::array<std::shared_ptr<KGL::Texture>, CUBE::NUM>	tex;
		std::array<KGL::DescriptorHandle, CUBE::NUM>			imgui_handle;
	};
public:
	static inline const std::vector<std::pair<std::string, std::string>> TEXTURES =
	{
		{ "Cartoon Base BlueSky",		"Sky_Day_BlueSky_Nothing_" },
		{ "Cartoon Base NightSky",		"Cartoon Base NightSky_" },
		{ "Cold Night",					"Cold Night__" },
		{ "Cold Sunset",				"Cold Sunset__" },
		{ "Deep Dusk",					"Deep Dusk__" },
		{ "Epic_BlueSunset",			"Epic_BlueSunset_" },
		{ "Epic_GloriousPink",			"Epic_GloriousPink_" },
		{ "Night MoonBurst",			"Night Moon Burst_" },
		{ "Overcast Low",				"Sky_AllSky_Overcast4_Low_" },
		{ "Space_AnotherPlanet",		"AllSky_Space_AnotherPlanet_" },
	};
private:
	float														scale;
	std::array<D3D12_VERTEX_BUFFER_VIEW, CUBE::NUM>				vbv;
	std::map<std::string, std::shared_ptr<Tex>>					tex_data;
	std::shared_ptr<KGL::Resource<Vertex>>						vbr;
	std::vector<std::shared_ptr<KGL::BaseRenderer>>				renderers;
	std::shared_ptr<KGL::Resource<DirectX::XMFLOAT4X4>>			buffer;
	std::shared_ptr<KGL::DescriptorManager>						desc_mgr;
	KGL::DescriptorHandle										buffer_handle;

	std::shared_ptr<Tex>										select;
	bool														render_flg;
private:
	void Change(bool next);
public:
	SkyManager(KGL::ComPtrC<ID3D12Device> device,
		const std::shared_ptr<KGL::DXC>& dxc, 
		std::shared_ptr<KGL::DescriptorManager> imgui_desc_mgr,
		DXGI_SAMPLE_DESC max_sample_desc,
		std::string folder = "./Assets/Textures/Sky/",
		const std::vector<std::pair<std::string, std::string>>& textures = TEXTURES,
		std::string extension = ".DDS");
	void Init(DirectX::CXMMATRIX viewproj);
	void UpdateGui();
	void Update(const DirectX::XMFLOAT3& pos, DirectX::CXMMATRIX viewproj);
	void SetWVP(DirectX::CXMMATRIX wvp);
	void Render(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list, UINT msaa_scale);
	void Uninit(std::shared_ptr<KGL::DescriptorManager> imgui_desc_mgr);
	void ChangeNext() { Change(true); }
	void ChangeBack() { Change(false); }
};
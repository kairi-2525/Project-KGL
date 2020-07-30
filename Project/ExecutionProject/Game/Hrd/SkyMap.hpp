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
private:
	float														scale;
	std::array<D3D12_VERTEX_BUFFER_VIEW, CUBE::NUM>				vbv;
	std::map<std::string, std::shared_ptr<Tex>>					tex_data;
	std::shared_ptr<KGL::Resource<Vertex>>						vbr;
	std::shared_ptr<KGL::BaseRenderer>							renderer;
	std::shared_ptr<KGL::Resource<DirectX::XMFLOAT4X4>>			buffer;
	std::shared_ptr<KGL::DescriptorManager>						tex_desc_mgr;

	std::shared_ptr<Tex>										select;

	SkyManager(std::string folder, std::string name1, std::string name2);
	void Update();
	void Render(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list);
};
#pragma once

#include <DirectXMath.h>
#include <Dx12/3D/Renderer.hpp>
#include <Dx12/DescriptorHeap.hpp>
#include <Dx12/ConstantBuffer.hpp>
#include <vector>

class DebugManager
{
public:
	struct CBuffer
	{
		DirectX::XMFLOAT4X4 view_projection;
	};

	struct Vertex
	{
		DirectX::XMFLOAT3	pos;
		DirectX::XMFLOAT4	color;
	};

	struct Object
	{
		virtual ~Object() = default;
		virtual size_t GetVertex(Vertex* out) const = 0;
	};

	struct Cube : public Object
	{
		static const std::vector<DirectX::XMFLOAT3> ModelVertex;

		DirectX::XMFLOAT3	pos;
		DirectX::XMFLOAT3	scale;
		DirectX::XMFLOAT3	rotate;
		DirectX::XMFLOAT4	color;

		size_t GetVertex(Vertex* out) const override;
	};

private:
	bool									s_obj_wire;
	std::shared_ptr<KGL::BaseRenderer>		s_obj_renderer;
	std::shared_ptr<KGL::BaseRenderer>		s_obj_wire_renderer;
	std::shared_ptr<KGL::DescriptorManager> s_obj_descmgr;
	KGL::DescriptorHandle					s_obj_handle;
	std::shared_ptr<KGL::Resource<CBuffer>>	s_obj_buffer_resource;
	std::shared_ptr<KGL::Resource<Vertex>>	s_obj_vertex_resource;
	D3D12_VERTEX_BUFFER_VIEW				s_obj_vertex_view;
	std::vector<Object>						s_objects;
	bool									s_obj_changed;
private:
	HRESULT UpdateStaticObjects();
public:
	DebugManager(ComPtrC<ID3D12Device> device, std::shared_ptr<KGL::BASE::DXC> dxc);
	void AddStaticObjects(const std::vector<Object>& objects);
	HRESULT Update(DirectX::CXMMATRIX view_proj);
	void Render(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list);
};
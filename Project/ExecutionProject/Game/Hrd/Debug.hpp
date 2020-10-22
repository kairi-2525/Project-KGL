#pragma once

#include <DirectXMath.h>
#include <Dx12/3D/Renderer.hpp>
#include <Dx12/DescriptorHeap.hpp>
#include <Dx12/ConstantBuffer.hpp>
#include <Dx12/Texture.hpp>
#include <vector>

class DebugManager
{
public:
	struct TransformConstants
	{
		DirectX::XMFLOAT4X4 view_projection;
		DirectX::XMFLOAT4X4 sky_projection;
	};
	struct ShadingConstants
	{
		struct Light
		{
			DirectX::XMFLOAT3 direction; float pad0;
			DirectX::XMFLOAT3 radiance; float pad1;
		} lights[3];
		DirectX::XMFLOAT3 eye_position;
	};

	struct Vertex
	{
		DirectX::XMFLOAT3	pos;
		DirectX::XMFLOAT3	normal;
		/*DirectX::XMFLOAT3	tangent;
		DirectX::XMFLOAT3	bitangent;*/
		DirectX::XMFLOAT2	texcoord;
	};

	struct Object
	{
		virtual ~Object() = default;
		virtual size_t GetVertex(Vertex* out) const = 0;
		virtual size_t GetVertexCount() const = 0;
	};

	struct Cube : public Object
	{
		static const std::vector<Vertex> ModelVertex;

		DirectX::XMFLOAT3	pos;
		DirectX::XMFLOAT3	scale;
		DirectX::XMFLOAT3	rotate;
		DirectX::XMFLOAT4	color;

		size_t GetVertex(Vertex* out) const override;
		size_t GetVertexCount() const override { return ModelVertex.size(); }
	};
private:
	struct Texture
	{
		std::shared_ptr<KGL::Texture>		tex;
		KGL::DescriptorHandle				handle;
	};
private:
	bool									render_flg;
	bool									s_obj_wire;
	std::vector<std::shared_ptr<KGL::BaseRenderer>>		s_obj_renderers;
	std::vector<std::shared_ptr<KGL::BaseRenderer>>		s_obj_wire_renderers;
	std::shared_ptr<KGL::DescriptorManager> s_obj_cbv_descmgr;
	std::shared_ptr<KGL::DescriptorManager> s_obj_srv_descmgr;
	KGL::DescriptorHandle					s_obj_tc_handle;
	KGL::DescriptorHandle					s_obj_sc_handle;
	std::shared_ptr<KGL::Resource<TransformConstants>>	s_obj_tc_resource;
	std::shared_ptr<KGL::Resource<ShadingConstants>>	s_obj_sc_resource;
	std::shared_ptr<KGL::Resource<Vertex>>	s_obj_vertex_resource;
	D3D12_VERTEX_BUFFER_VIEW				s_obj_vertex_view;
	std::vector<std::shared_ptr<Object>>	s_objects;
	bool									s_obj_changed;
	UINT									s_obj_vertices_offset;

	std::shared_ptr<KGL::Texture>			white_texture;
	std::shared_ptr<KGL::Texture>			black_texture;
	Texture									s_obj_albedo;
	Texture									s_obj_normal;
	Texture									s_obj_metalness;
	Texture									s_obj_roughness;
	Texture									s_obj_specular;
	Texture									s_obj_irradiance;
	Texture									s_obj_specular_brdf;
private:
	HRESULT UpdateStaticObjects();
	void ChangeTexture(Texture* texture, int flg);
public:
	DebugManager(ComPtrC<ID3D12Device> device, std::shared_ptr<KGL::BASE::DXC> dxc, DXGI_SAMPLE_DESC max_sample_desc);
	void AddStaticObjects(const std::vector<std::shared_ptr<Object>>& objects);
	void ClearStaticObjects();
	HRESULT Update(const TransformConstants& tc, const ShadingConstants& sc, bool use_gui);
	void Render(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list, UINT msaa_count);
	void SetWireMode(bool wire) { s_obj_wire = wire; }
	bool GetWireMode() { return s_obj_wire; }
	void SetRenderFlg(bool render_flg) { this->render_flg = render_flg; }
	bool GetRenderFlg() const { return render_flg; }
};
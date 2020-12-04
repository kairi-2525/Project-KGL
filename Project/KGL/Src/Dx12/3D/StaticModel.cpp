#include <Dx12/3D/StaticModel.hpp>

using namespace KGL;

StaticModel::StaticModel(
	ComPtrC<ID3D12Device> device,
	std::shared_ptr<const StaticModelLoader> loader,
	std::shared_ptr<TextureManager> tex_mgr,
	std::shared_ptr<DescriptorManager> descriptor_mgr
) noexcept
{
	if (!loader) return;
	auto materials = loader->GetMaterials();
	if (!materials) return;

	m_path = loader->GetPath();

	// Loaderが持つ情報からGPUリソースを構築する
	auto mtl_size = materials->size();
	m_materials.reserve(mtl_size);

	// 外部descriptor_mgrをセット
	if (descriptor_mgr)
		m_descriptor_mgr = descriptor_mgr;
	else
		m_descriptor_mgr = std::make_shared<DescriptorManager>(device, mtl_size * (TEXTURE_SIZE + 1));

	// 外部TexMgrをセット
	if (tex_mgr)
		m_tex_mgr = tex_mgr;
	else
		m_tex_mgr = std::make_shared<TextureManager>();

	auto white_tex = std::make_shared<Texture>(device, 0xff, 0xff, 0xff, 0xff, m_tex_mgr.get());
	auto black_tex = std::make_shared<Texture>(device, 0x00, 0x00, 0x00, 0xff, m_tex_mgr.get());
	auto y_up_tex = std::make_shared<Texture>(device, 0xff / 2, 0xff, 0xff / 2, 0x00, m_tex_mgr.get());

	constexpr auto CreateTexture = [](
		ComPtrC<ID3D12Device>				device,
		HandleTexture*						out_handletex,
		const std::filesystem::path&		path,
		const std::shared_ptr<Texture>		empty_tex,
		std::shared_ptr<TextureManager>		tex_mgr,
		std::shared_ptr<DescriptorManager>	descriptor_mgr
		) {
			if (path.empty())
			{
				out_handletex->tex = empty_tex;
			}
			else
			{
				out_handletex->tex = std::make_shared<Texture>(device, path, 1u, tex_mgr.get());
			}

			out_handletex->handle = std::make_shared<DescriptorHandle>(descriptor_mgr->Alloc());
			out_handletex->tex->CreateSRVHandle(out_handletex->handle);
		};
	constexpr auto CreateReflectionTexture = [](
		ComPtrC<ID3D12Device>						device,
		HandleTexture*								out_handletex,
		const S_MODEL::Textures::ReflectionsPath&	data,
		const std::string&							name,
		const std::shared_ptr<Texture>				empty_tex,
		std::shared_ptr<TextureManager>				tex_mgr,
		std::shared_ptr<DescriptorManager>			descriptor_mgr
		) {
		if (data.count(name) == 0u)
		{
			out_handletex->tex = empty_tex;
		}
		else
		{
			out_handletex->tex = std::make_shared<Texture>(device, data.at(name), 1u, tex_mgr.get());
		}

		out_handletex->handle = std::make_shared<DescriptorHandle>(descriptor_mgr->Alloc());
		out_handletex->tex->CreateSRVHandle(out_handletex->handle);
	};

	for (auto& it : *materials)
	{
		const auto& load_mt = it.second;
		auto& mt = m_materials[it.first];
		auto& param_handle = m_param_handles[it.first];

		// パラメータバッファ
		mt.rs_param = std::make_shared<Resource<S_MODEL::Material::Parameter>>(device, 1u);
		{
			auto* mapped_param = mt.rs_param->Map();
			*mapped_param = load_mt.param;
			mt.rs_param->Unmap();
		}
		mt.param_cbv_handle = std::make_shared<DescriptorHandle>(m_descriptor_mgr->Alloc());
		mt.rs_param->CreateCBV(mt.param_cbv_handle);

		//頂点用GPUリソースを作成
		mt.rs_vertices = std::make_shared<Resource<S_MODEL::Vertex>>(device, load_mt.vertices.size());
		{
			auto* mapped_vertices = mt.rs_vertices->Map();
			std::copy(load_mt.vertices.cbegin(), load_mt.vertices.cend(), mapped_vertices);
			mt.rs_vertices->Unmap();
		}
		// 頂点用ビュー
		mt.vertex_buffer_view.BufferLocation = mt.rs_vertices->Data()->GetGPUVirtualAddress();
		mt.vertex_buffer_view.SizeInBytes = SCAST<UINT>(mt.rs_vertices->SizeInBytes());
		mt.vertex_buffer_view.StrideInBytes = sizeof(S_MODEL::Vertex);

		// テクスチャ用GPUリソースとSRVを作成
		CreateTexture(device, &mt.ambient, load_mt.tex.ambient, white_tex, m_tex_mgr, m_descriptor_mgr);
		CreateTexture(device, &mt.diffuse, load_mt.tex.diffuse, white_tex, m_tex_mgr, m_descriptor_mgr);
		CreateTexture(device, &mt.specular, load_mt.tex.specular, white_tex, m_tex_mgr, m_descriptor_mgr);
		CreateTexture(device, &mt.specular_highlights, load_mt.tex.specular_highlights, white_tex, m_tex_mgr, m_descriptor_mgr);
		CreateTexture(device, &mt.dissolve, load_mt.tex.dissolve, white_tex, m_tex_mgr, m_descriptor_mgr);
		CreateTexture(device, &mt.bump, load_mt.tex.bump, y_up_tex, m_tex_mgr, m_descriptor_mgr);
		CreateTexture(device, &mt.displacement, load_mt.tex.displacement, white_tex, m_tex_mgr, m_descriptor_mgr);
		CreateTexture(device, &mt.stencil_decal, load_mt.tex.stencil_decal, white_tex, m_tex_mgr, m_descriptor_mgr);
		CreateReflectionTexture(device, &mt.reflections[SPHERE], load_mt.tex.reflections, "sphere", white_tex, m_tex_mgr, m_descriptor_mgr);
		CreateReflectionTexture(device, &mt.reflections[CUBE_TOP], load_mt.tex.reflections, "cube_top", white_tex, m_tex_mgr, m_descriptor_mgr);
		CreateReflectionTexture(device, &mt.reflections[CUBE_BOTTOM], load_mt.tex.reflections, "cube_bottom", white_tex, m_tex_mgr, m_descriptor_mgr);
		CreateReflectionTexture(device, &mt.reflections[CUBE_FRONT], load_mt.tex.reflections, "cube_front", white_tex, m_tex_mgr, m_descriptor_mgr);
		CreateReflectionTexture(device, &mt.reflections[CUBE_BACK], load_mt.tex.reflections, "cube_back", white_tex, m_tex_mgr, m_descriptor_mgr);
		CreateReflectionTexture(device, &mt.reflections[CUBE_LEFT], load_mt.tex.reflections, "cube_left", white_tex, m_tex_mgr, m_descriptor_mgr);
		CreateReflectionTexture(device, &mt.reflections[CUBE_RIGHT], load_mt.tex.reflections, "cube_right", white_tex, m_tex_mgr, m_descriptor_mgr);
	}
}

void StaticModel::Render(ComPtrC<ID3D12GraphicsCommandList> cmd_list) const noexcept
{
	for (const auto& it : m_materials)
	{
		const auto& mt = it.second;

		cmd_list->SetDescriptorHeaps(1, mt.param_cbv_handle->Heap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, mt.param_cbv_handle->Gpu());

		cmd_list->SetDescriptorHeaps(1, mt.ambient.handle->Heap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(1, mt.ambient.handle->Gpu());

		cmd_list->IASetVertexBuffers(0, 1, &mt.vertex_buffer_view);
		cmd_list->DrawInstanced(SCAST<UINT>(mt.rs_vertices->Size()), 1, 0, 0);
	}
}
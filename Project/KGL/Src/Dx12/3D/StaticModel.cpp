#include <Dx12/3D/StaticModel.hpp>

using namespace KGL;

StaticModel::StaticModel(
	ComPtrC<ID3D12Device> device,
	std::shared_ptr<const StaticModelLoader> loader
) noexcept
{
	if (!loader) return;
	auto materials = loader->GetMaterials();
	if (!materials) return;

	// Loader������񂩂�GPU���\�[�X���\�z����
	m_materials.reserve(materials->size());
	for (auto& it : *materials)
	{
		const auto& load_mt = it.second;
		auto& mt = m_materials.emplace_back();
		
		mt.rs_vertices = std::make_shared<Resource<S_MODEL::Vertex>>(device, load_mt.vertices.size());
	}
}

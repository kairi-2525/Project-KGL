#include <Dx12/3D/StaticModel.hpp>

using namespace KGL;

StaticModel::StaticModel(std::shared_ptr<const StaticModelLoader> loader) noexcept
{
	if (!loader) return;
	auto materials = loader->GetMaterials();
	if (!materials) return;

	for (auto& mt : *materials)
	{
		mt.second.vertices.size();
	}
}
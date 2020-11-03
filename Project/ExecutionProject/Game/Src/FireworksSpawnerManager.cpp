#include "../Hrd/FireworksSpawnerManager.hpp"
#include <Cereal/archives/binary.hpp>
#include <Cereal/types/vector.hpp>
#include <Cereal/types/memory.hpp>
#include <fstream>
#include <imgui.h>

void FS_Obj::Init(const std::map<const std::string, std::shared_ptr<FireworksDesc>>& desc_list)
{
	if (desc_list.count(obj_desc.fw_name) == 1u)
	{
		fw_desc = desc_list.at(obj_desc.fw_name);
	}
}

void FS_Obj::Update(float update_time, std::vector<Fireworks>* pout_fireworks)
{

}

void FS::Init(const std::map<const std::string, std::shared_ptr<FireworksDesc>>& desc_list)
{
	for (auto& obj : objects)
	{
		obj.Init(desc_list);
	}
}

void FS::Update(float update_time, std::vector<Fireworks>* pout_fireworks)
{
	for (auto& obj : objects)
	{
		obj.Update(update_time, pout_fireworks);
	}
}

FSManager::FSManager(const std::filesystem::path& path, const std::map<const std::string, std::shared_ptr<FireworksDesc>>& desc_list) noexcept :
	KGL::Directory(path)
{
	KGL::Files files = GetFiles(".bin");
	const std::string dir_path = GetPath().string();
	const size_t file_size = files.size();

	// spawnerÇì«Ç›çûÇﬁ
	for (size_t i = 0u; i < file_size; i++)
	{
		std::ifstream ifs(dir_path + files[i].string(), std::ios::binary);
		if (ifs.is_open())
		{
			const auto& file_name = files[i].filename().stem();
			std::shared_ptr<FS> desc;
			cereal::BinaryInputArchive i_archive(ifs);
			i_archive(KGL_NVP(file_name.string(), desc));
			desc->Init(desc_list);
			fs_list.push_back(desc);
		}
	}
}

void FSManager::Update(float update_time, std::vector<Fireworks>* pout_fireworks)
{
	// ëIëíÜÇÃFSÇ©ÇÁFireworksÇî≠ê∂Ç≥ÇπÇÈ
	if (select_fs)
	{
		select_fs->Update(update_time, pout_fireworks);
	}
}

void FSManager::GUIUpdate()
{
	if (ImGui::Begin("Fireworks Spawner"))
	{
		
	}
	ImGui::End();
}
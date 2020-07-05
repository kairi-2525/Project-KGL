#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <Loader/VMDLoader.hpp>
#include <Helper/ThrowAssert.hpp>
#include <fstream>

using namespace KGL;

VMD_Loader::VMD_Loader(std::filesystem::path path) noexcept
{
	using namespace DirectX;
	std::ifstream ifs(path, std::ios::in | std::ios::binary);
	try
	{
		if (!ifs.is_open()) throw std::runtime_error("[ " + path.string() + " ] が見つかりませんでした。");
		
		// ヘッダーをスキップ
		ifs.seekg(50);

		UINT motion_data_num = 0;
		ifs.read((char*)&motion_data_num, sizeof(motion_data_num));

		m_desc.motions.resize(motion_data_num);
		ifs.read((char*)m_desc.motions.data(),
			sizeof(VMD::Motion) * motion_data_num
		);

		// VMDのモーションデータから、実際に使用するモーションテーブルへ変換
		for (auto& motion : m_desc.motions)
		{
			m_desc.motion_data[motion.bone_name].emplace_back(
				motion.frame_no,
				XMLoadFloat4(&motion.quaternion)
			);
		}
	}
	catch (std::runtime_error& exception)
	{
		RuntimeErrorStop(exception);
	}
}
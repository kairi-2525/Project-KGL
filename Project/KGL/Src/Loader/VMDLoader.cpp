#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <Loader/VMDLoader.hpp>
#include <Helper/ThrowAssert.hpp>
#include <fstream>

using namespace KGL;

namespace // 無名名前空間
{
	enum class BoneType
	{
		Rotation,			// 回転
		RotAndMove,			// 回転＆移動
		IK,					// IK
		Undefined,			// 未定義
		IKChild,			// IK影響ボーン
		RotationChild,		// 回転影響ボーン
		IKDestination,		// IK接続先
		Invisible			// 見えないボーン
	};
}

VMD_Loader::VMD_Loader(std::filesystem::path path) noexcept
{
	using namespace DirectX;
	std::ifstream ifs(path, std::ios::in | std::ios::binary);
	try
	{
		if (!ifs.is_open()) throw std::runtime_error("[ " + path.string() + " ] が見つかりませんでした。");
		std::shared_ptr<VMD::Desc> desc = std::make_shared<VMD::Desc>();
		m_desc = desc;
		// ヘッダーをスキップ
		ifs.seekg(50);

		UINT motion_data_num = 0;
		ifs.read((char*)&motion_data_num, sizeof(motion_data_num));

		desc->motions.resize(motion_data_num);
		ifs.read((char*)desc->motions.data(),
			sizeof(VMD::Motion) * motion_data_num
		);

		// VMDのモーションデータから、実際に使用するモーションテーブルへ変換
		desc->max_frame = 0u;
		for (auto& motion : desc->motions)
		{
			desc->max_frame = std::max(motion.frame_no, desc->max_frame);
			desc->motion_data[motion.bone_name].emplace_back(
				motion.frame_no,
				XMLoadFloat4(&motion.quaternion),
				motion.location,
				XMFLOAT2{ motion.bezier[3] / 127.f, motion.bezier[7] / 127.f },
				XMFLOAT2{ motion.bezier[11] / 127.f, motion.bezier[15] / 127.f }
			);
		}

		// フレーム番号順にソート
		for (auto& motion : desc->motion_data)
		{
			std::sort(
				motion.second.begin(),
				motion.second.end(),
				[](const VMD::Key_Frame lval, const VMD::Key_Frame& rval)
				{
					return lval.frame_no <= rval.frame_no;
				}
			);
		}
	}
	catch (std::runtime_error& exception)
	{
		RuntimeErrorStop(exception);
	}
}
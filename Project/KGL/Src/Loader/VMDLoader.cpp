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

		UINT motion_data_num = 0u;
		ifs.read((char*)&motion_data_num, sizeof(motion_data_num));

		desc->motions.resize(motion_data_num);
		ifs.read((char*)desc->motions.data(),
			sizeof(VMD::Motion) * motion_data_num
		);

		UINT32 morph_count = 0u;
		ifs.read((char*)&morph_count, sizeof(morph_count));
		desc->morphs.resize(morph_count);
		ifs.read((char*)desc->morphs.data(), morph_count * sizeof(VMD::Morph));

		UINT32 camera_count = 0u;
		ifs.read((char*)&camera_count, sizeof(camera_count));
		desc->cameras.resize(camera_count);
		ifs.read((char*)desc->cameras.data(), camera_count * sizeof(VMD::Camera));

		UINT32 light_count = 0u;
		ifs.read((char*)&light_count, sizeof(light_count));
		desc->lights.resize(light_count);
		ifs.read((char*)desc->lights.data(), light_count * sizeof(VMD::Light));

		UINT32 self_shadow_count = 0u;
		ifs.read((char*)&self_shadow_count, sizeof(self_shadow_count));
		desc->self_shadows.resize(self_shadow_count);
		ifs.read((char*)desc->self_shadows.data(), self_shadow_count * sizeof(VMD::SelfShadow));

		UINT32 is_switch_count = 0u;
		ifs.read((char*)&is_switch_count, sizeof(is_switch_count));
		desc->ik_enable_data.resize(is_switch_count);
		for (auto& ik_enable : desc->ik_enable_data)
		{
			// フレーム番号読み込み
			ifs.read((char*)&ik_enable.frame_no, sizeof(ik_enable.frame_no));
			// 次に仮想フラグがあるが、これは使用しないため１バイトシークでも構わない
			UINT8 visivle_flg = 0u;
			ifs.read((char*)&visivle_flg, sizeof(visivle_flg));

			// 対象ボーン数読み込み
			UINT32 ik_bone_count = 0u;
			ifs.read((char*)&ik_bone_count, sizeof(ik_bone_count));
			// ループしつつ名前とON / OFF情報を取得
			for (UINT32 i = 0u; i < ik_bone_count; i++)
			{
				char ik_bone_name[20];
				ifs.read(ik_bone_name, std::size(ik_bone_name));

				UINT8 flg = 0u;
				ifs.read((char*)&flg, sizeof(flg));
				ik_enable.ik_enable_table[ik_bone_name] = flg != 0u;
			}
		}



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
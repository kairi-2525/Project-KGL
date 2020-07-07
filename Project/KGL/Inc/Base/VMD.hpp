#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <map>
//#include <unordered_map>

namespace KGL
{
	inline namespace BASE
	{
		namespace VMD
		{
			/*struct Header
			{
				FLOAT	version;
				CHAR	model_name[20];
				CHAR	comment[256];
			};*/
#pragma pack(1)
			struct Motion
			{
				CHAR				bone_name[15];
				UINT				frame_no;
				DirectX::XMFLOAT3	location;
				DirectX::XMFLOAT4	quaternion;
				UCHAR				bezier[64];		// [4][4][4] ベジェ保管パラメーター
			};

			//表情データ(頂点モーフデータ)
			struct Morph
			{
				char		name[15];
				UINT32		frame_no;
				float		weight;		// 0.0f ~ 1.0f
			};	// 23バイト

			struct Camera
			{
				UINT32				frame_no;
				FLOAT				distance;
				DirectX::XMFLOAT3	pos;
				DirectX::XMFLOAT3	euler_angle;
				UINT8				interpolation[24];	// 補間
				UINT32				fov;
				UINT8				pers_flg;			// ON / OFF
			};	// 61バイト
			
			// セルフ影データ
			struct SelfShadow
			{
				UINT32	frame_no;
				UINT8	mode;		// 0:影無し、1:モード１、2:モード2
				FLOAT	distance;
			};
#pragma pack()
			struct Light
			{
				UINT32 frame_no;		// フレーム番号
				DirectX::XMFLOAT3 rgb;
				DirectX::XMFLOAT3 vec;	// 光線ベクトル(平行光線)
			};
			struct IKEnable
			{
				// キーフレームがあるフレーム番号
				UINT32 frame_no;
				std::map<std::string, bool> ik_enable_table;
			};

			struct Key_Frame
			{
				UINT				frame_no;	// アニメーション開始からのフレーム数
				DirectX::XMVECTOR	quaternion;
				DirectX::XMFLOAT3	offset;		// IKの初期座標
				DirectX::XMFLOAT2	p1, p2;		// ベジェ曲線の中間コントロールポイント

				explicit Key_Frame(UINT fno, DirectX::CXMVECTOR q,
					const DirectX::XMFLOAT3& offset,
					const DirectX::XMFLOAT2& p1, const DirectX::XMFLOAT2& p2) noexcept
					: frame_no(fno), quaternion(q), offset(offset), p1(p1), p2(p2) {}
			};

			struct Desc
			{
				std::vector<Motion>								motions;
				std::map<std::string, std::vector<Key_Frame>>	motion_data;
				UINT											max_frame;
				std::vector<Morph>								morphs;
				std::vector<Camera>								cameras;
				std::vector<Light>								lights;
				std::vector<SelfShadow>							self_shadows;
				std::vector<IKEnable>							ik_enable_data;
			};
		}
	}
}
#pragma once

#include <DirectXMath.h>
#include <vector>

struct Effect
{
	// XMFLOAT2�͑S�� �ŏ��lx ~ �ő�ly

	float				start_time;				// start_time�o�ߌ�o�����܂��B
	float				time;					// start_time�o�ߌ�time�b�Ԍ���ł��܂��B
	float				start_accel;			// start_time���o�߂����u�Ԃ�Firework�N���X�� ����
	float				end_accel;				// start_time + time���o�߂����u�Ԃ�Firework�N���X��
												// ���x��accel�{�ɂ��邱�Ƃ��ł��܂��B
	DirectX::XMFLOAT2	alive_time;				// �p�[�e�B�N���̕\������
	DirectX::XMFLOAT2	late;					// ��b�Ԃ�late�̃p�[�e�B�N�����������܂�
	float				late_update_time;		// late_update_time���o�߂����late���ēx�v�Z���Ȃ����܂��B
	DirectX::XMFLOAT2	speed;					// �p�[�e�B�N���ˏo���̑��x
	DirectX::XMFLOAT2	scale;					// �p�[�e�B�N���ˏo���̑傫��
	DirectX::XMFLOAT2	angle;					// �p�[�e�B�N���ˏo�p�x
	DirectX::XMFLOAT2	spawn_space;			// �p�[�e�B�N���ˏo������spawn_space���ʒu�����炵�܂��B
	DirectX::XMFLOAT4	color;					// �F
};

namespace FIREWORK_EFFECTS
{
	extern inline const std::vector<Effect> A =
	{
		{ 
			0.f,	// start_time
			5.f,	// time
			1.f,	// start_accel
			1.f,	// end_accel
			{ 2.f, 3.f },	// alive_time
			{ 100.f, 1000.f },	// late
			0.2f,			// late_update_time
			{ 0.1f, 0.1f },	// speed
			{ 0.05f, 0.1f },	// scale
			{ DirectX::XMConvertToRadians(120.f) , DirectX::XMConvertToRadians(180.f) },	// angle
			{ 0.5f, 0.5f },	// spawn_space
			{ 1.f, 1.f, 1.f, 0.1f },	// color

		},
		{
			5.f,	// start_time
			0.1f,	// time
			0.f,	// start_accel
			0.f,	// end_accel
			{ 2.f, 3.f },	// alive_time
			{ 100000.f, 100000.f },	// late
			1.0f,			// late_update_time
			{ 5.f, 5.f },	// speed
			{ 0.05f, 0.1f },	// scale
			{ DirectX::XMConvertToRadians(0.f) , DirectX::XMConvertToRadians(180.f) },	// angle
			{ 0.1f, 0.1f },	// spawn_space
			{ 1.f, 1.f, 1.f, 0.1 },	// color

		}
	};
}
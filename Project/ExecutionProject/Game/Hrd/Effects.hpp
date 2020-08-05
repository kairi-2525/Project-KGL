#pragma once

#include <DirectXMath.h>
#include <vector>
#include <memory>

struct Particle;
struct ParticleParent;
class Fireworks;
struct EffectDesc;

struct FireworksDesc
{
	DirectX::XMFLOAT3	pos;
	DirectX::XMFLOAT3	velocity;
	float				mass;

	std::vector<EffectDesc>	effects;
};

struct EffectDesc
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
	DirectX::XMFLOAT2	base_speed;					// �p�[�e�B�N���ˏo���̎ˏo�����x�̉e���x
	DirectX::XMFLOAT2	scale;					// �p�[�e�B�N���ˏo���̑傫��
	float				scale_front, scale_back;// scale��speed�ɉe�����󂯂�ړ������ւ̃X�P�[��
	DirectX::XMFLOAT2	angle;					// �p�[�e�B�N���ˏo�p�x
	DirectX::XMFLOAT2	spawn_space;			// �p�[�e�B�N���ˏo������spawn_space���ʒu�����炵�܂��B
	DirectX::XMFLOAT4	begin_color;			// �J�n���_�ł̐F
	DirectX::XMFLOAT4	end_color;				// �I�����_�ł̐F
	bool				bloom;					// �u���[���������邩�ǂ����B
	
	bool				has_child;				// ���̃t���O���m�F���ā���K�����܂�
	FireworksDesc		child;					// �p�[�e�B�N���̑����Fireworks���쐬����ꍇ�����Ɏw�肵�܂��B

};

struct Effect
{
	EffectDesc effect;
	float total_time_count;
	float update_timer;
	float late_counter;
	float late;

	void Init(const EffectDesc& desc);
	void Update(DirectX::CXMVECTOR pos, DirectX::CXMVECTOR velocity,
		float time, std::vector<Particle>* p_particles, const ParticleParent* p_parent,
		std::vector<Fireworks>* p_fireworks);
};

namespace FIREWORK_EFFECTS
{
	extern inline const std::vector<EffectDesc> A =
	{
		{
			0.f,	// start_time
			7.f,	// time
			1.f,	// start_accel
			1.f,	// end_accel
			{ 2.f, 3.f },	// alive_time
			{ 200.f, 200.f },	// late
			0.2f,			// late_update_time
			{ 0.1f, 0.1f },	// speed
			{ 0.f, 0.f },	// base_speed
			{ 0.2f, 1.0f },	// scale
			0.f, 0.f,		// scale_front, scale_back
			{ DirectX::XMConvertToRadians(177.f) , DirectX::XMConvertToRadians(180.f) },	// angle
			{ 0.1f, 0.2f },	// spawn_space
			{ 0.1f, 0.1f, 0.1f, 0.05f },	// begin_color
			{ 0.1f, 0.1f, 0.1f, 0.05f },	// end_color
			false,							// bloom
			false
		},
		{
			7.0f,	// start_time
			0.2f,	// time
			1.f,	// start_accel
			1.f,	// end_accel
			{ 1.f, 2.f },	// alive_time
			{ 1000.f, 2000.f },	// late
			1.0f,			// late_update_time
			{ 15.f, 20.f },	// speed
			{ 0.f, 0.f },	// base_speed
			{ 0.8f, 1.2f },	// scale
			0.f, 0.f,		// scale_front, scale_back
			{ DirectX::XMConvertToRadians(0.f) , DirectX::XMConvertToRadians(180.f) },	// angle
			{ 0.5f, 0.5f },	// spawn_space
			{ 1.f, 0.f, 0.5f, 0.1f },		// begin_color
			{ 1.f, 0.f, 0.5f, 0.0f },		// end_color
			true,							// bloom
			false
		},
		{
			7.f,	// start_time
			0.2f,	// time
			0.f,	// start_accel
			1.f,	// end_accel
			{ 1.f, 2.f },	// alive_time
			{ 5000.f, 5000.f },	// late
			1.0f,			// late_update_time                   
			{ 25.f, 35.f },	// speed
			{ 0.f, 0.f },	// base_speed
			{ 0.5f, 1.2f },	// scale
			0.f, 0.f,		// scale_front, scale_back
			{ DirectX::XMConvertToRadians(0.f) , DirectX::XMConvertToRadians(180.f) },	// angle
			{ 0.1f, 0.1f },	// spawn_space
			{ 1.f, 0.5f, 0.f, 0.1f },		// begin_color
			{ 1.f, 0.5f, 0.f, 0.0f },		// end_color
			true,							// bloom
			false
		}
	};
}
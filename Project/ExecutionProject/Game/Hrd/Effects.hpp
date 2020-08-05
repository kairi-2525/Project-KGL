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
	// XMFLOAT2は全て 最小値x ~ 最大値y

	float				start_time;				// start_time経過後出現します。
	float				time;					// start_time経過後time秒間後消滅します。
	float				start_accel;			// start_timeが経過した瞬間にFireworkクラスで ↓↓
	float				end_accel;				// start_time + timeが経過した瞬間にFireworkクラスで
												// 速度をaccel倍にすることができます。
	DirectX::XMFLOAT2	alive_time;				// パーティクルの表示時間
	DirectX::XMFLOAT2	late;					// 一秒間にlate個のパーティクルが発生します
	float				late_update_time;		// late_update_timeが経過するとlateを再度計算しなおします。
	DirectX::XMFLOAT2	speed;					// パーティクル射出時の速度
	DirectX::XMFLOAT2	base_speed;					// パーティクル射出時の射出元速度の影響度
	DirectX::XMFLOAT2	scale;					// パーティクル射出時の大きさ
	float				scale_front, scale_back;// scaleとspeedに影響を受ける移動方向へのスケール
	DirectX::XMFLOAT2	angle;					// パーティクル射出角度
	DirectX::XMFLOAT2	spawn_space;			// パーティクル射出方向へspawn_space分位置をずらします。
	DirectX::XMFLOAT4	begin_color;			// 開始時点での色
	DirectX::XMFLOAT4	end_color;				// 終了時点での色
	bool				bloom;					// ブルームをかけるかどうか。
	
	bool				has_child;				// このフラグを確認して↓を適応します
	FireworksDesc		child;					// パーティクルの代わりにFireworksを作成する場合ここに指定します。

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
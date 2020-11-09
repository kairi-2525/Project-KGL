#pragma once

#include <DirectXMath.h>
#include <vector>
#include <memory>
#include <random>
#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

enum class EFFECT_VERSION : UINT
{
	EV_0 = 1u,
	EV_1,
	EV_2
};
enum class FIREWORKS_VERSION : UINT
{
	FV_0 = 1u,
	FV_1
};

struct Particle;
struct ParticleParent;
class Fireworks;
struct AffectObjects;
struct EffectDesc;

struct FireworksDesc
{
	DirectX::XMFLOAT3	pos;
	DirectX::XMFLOAT3	velocity;
	float				mass;
	float				resistivity;
	float				speed;

	std::vector<EffectDesc>	effects;

	std::string set_name;
	std::string original_name;
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
	DirectX::XMFLOAT2	base_speed;				// パーティクル射出時の射出元速度の影響度
	DirectX::XMFLOAT2	scale;					// パーティクル射出時の大きさ
	float				scale_front, scale_back;// scaleとspeedに影響を受ける移動方向へのスケール
	DirectX::XMFLOAT2	angle;					// パーティクル射出角度
	DirectX::XMFLOAT2	spawn_space;			// パーティクル射出方向へspawn_space分位置をずらします。
	DirectX::XMFLOAT4	begin_color;			// エフェクト開始時点での色
	DirectX::XMFLOAT4	end_color;				// エフェクト終了時点での色
	DirectX::XMFLOAT4	erase_color;			// パーティクル消滅時点での色
	float				resistivity;			// 不可の影響度（スケールから影響を受けない）
	float				scale_resistivity;		// 不可の影響度（スケールから影響を受ける）
	bool				bloom;					// ブルームをかけるかどうか。
	
	bool				has_child;				// このフラグを確認して↓を適応します
	FireworksDesc		child;					// パーティクルの代わりにFireworksを作成する場合ここに指定します。

	std::string			name;
	std::string			set_name;
	std::string			texture_name;
	UINT32				id;
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
		std::vector<Fireworks>* p_fireworks,
		const std::vector<AffectObjects>& affect_objects,
		const std::vector<Fireworks>& affect_fireworks);
};

namespace FIREWORK_EFFECTS
{
	extern inline const FireworksDesc FW_DEFAULT =
	{
		{ 0.f, 0.f, 0.f }, // pos
		{ 0.f, 0.f, 0.f },
		1.f, 1.f, 1.f,
		{
			{
				0.f,	// start_time
				1.f,	// time
				1.f,	// start_accel
				1.f,	// end_accel
				{ 1.f, 1.f },	// alive_time
				{ 100.f, 100.f },	// late
				1.f,			// late_update_time
				{ 10.f, 10.f },	// speed
				{ 0.f, 0.f },	// base_speed
				{ 1.0f, 1.0f },	// scale
				0.f, 0.f,		// scale_front, scale_back
				{ DirectX::XMConvertToRadians(0.f) , DirectX::XMConvertToRadians(180.f) },	// angle
				{ 0.f, 0.f },	// spawn_space
				{ 1.f, 1.f, 1.f, 0.5f },	// begin_color
				{ 1.f, 1.f, 1.f, 0.5f },	// end_color
				{ 1.f, 1.f, 1.f, 0.00f },	// erase_color
				5.f, 0.f,						// resistivity, scale_resistivity
				true,							// bloom
				false
			}
		},

	};
	extern inline const std::vector<EffectDesc> A =
	{
		{
			0.f,	// start_time
			7.f,	// time
			1.f,	// start_accel
			1.f,	// end_accel
			{ 4.f, 5.f },	// alive_time
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
			{ 0.1f, 0.1f, 0.1f, 0.00f },	// erase_color
			5.f, 0.f,						// resistivity, scale_resistivity
			false,							// bloom
			false
		},
		{
			7.0f,	// start_time
			0.2f,	// time
			1.f,	// start_accel
			1.f,	// end_accel
			{ 3.f, 4.f },	// alive_time
			{ 1000.f, 2000.f },	// late
			1.0f,			// late_update_time
			{ 150.f, 200.f },	// speed
			{ 0.f, 0.f },	// base_speed
			{ 0.8f, 1.2f },	// scale
			0.f, 0.f,		// scale_front, scale_back
			{ DirectX::XMConvertToRadians(0.f) , DirectX::XMConvertToRadians(180.f) },	// angle
			{ 0.5f, 0.5f },	// spawn_space
			{ 1.f, 0.f, 0.5f, 0.1f },		// begin_color
			{ 1.f, 0.f, 0.5f, 0.0f },		// end_color
			{ 1.f, 0.f, 0.5f, 0.0f },		// erase_color
			5.f, 0.f,						// resistivity, scale_resistivity
			true,							// bloom
			false
		},
		{
			7.f,	// start_time
			0.2f,	// time
			0.f,	// start_accel
			1.f,	// end_accel
			{ 3.f, 4.f },	// alive_time
			{ 5000.f, 5000.f },	// late
			1.0f,			// late_update_time                   
			{ 250.f, 350.f },	// speed
			{ 0.f, 0.f },	// base_speed
			{ 0.5f, 1.2f },	// scale
			0.f, 0.f,		// scale_front, scale_back
			{ DirectX::XMConvertToRadians(0.f) , DirectX::XMConvertToRadians(180.f) },	// angle
			{ 0.1f, 0.1f },	// spawn_space
			{ 1.f, 0.5f, 0.f, 0.1f },		// begin_color
			{ 1.f, 0.5f, 0.f, 0.0f },		// end_color
			{ 1.f, 0.5f, 0.f, 0.0f },		// erase_color
			5.f, 0.f,						// resistivity, scale_resistivity
			true,							// bloom
			false
		}
	};

	inline FireworksDesc Get(UINT type)
	{
		using namespace DirectX;

		std::random_device rd;
		std::mt19937 mt(rd());
		std::uniform_real_distribution<float> rmd_unorm(0.f, 1.f);

		FireworksDesc desc{};
		desc.effects = FIREWORK_EFFECTS::A;
		desc.mass = 1.f;
		desc.resistivity = 0.1f;
		static constexpr auto to_sec_m = [](float hour_km)->float
		{
			return ((hour_km * 1000.f) / 60.f) / 60.f;
		};
		desc.speed = to_sec_m(356.f);
		switch (type)
		{
		case 0:
		{
			desc.effects.pop_back();
			desc.effects[0].time = 5.f;
			desc.effects[1].start_time = 5.f;
			desc.effects[1].late = { 50.f, 100.f };
			desc.effects[1].child = desc;
			desc.effects[1].resistivity = 1.f;
			desc.effects[1].has_child = true;
			desc.effects[1].base_speed = { 1.f, 1.f };
			//desc.effects[1].child.effects[0].late = { 10.f, 10.f };
			desc.effects[1].child.effects[0].time = 2.f;

			desc.effects[1].child.effects[1].alive_time = { 2.5f, 3.5f };
			desc.effects[1].child.effects[1].late = { 10000.f, 12000.f };
			desc.effects[1].child.effects[1].start_time = 2.f;
			desc.effects[1].child.effects[1].time = 0.05f;
			desc.effects[1].child.effects[1].start_accel = 1.f;
			desc.effects[1].child.effects[1].speed = { 100.f, 100.f };
			desc.effects[1].child.effects[1].base_speed = { 0.f, 0.f };
			desc.effects[1].child.effects[1].scale = { 0.8f, 1.2f };

			XMStoreFloat4(&desc.effects[1].child.effects[1].begin_color, XMVector3Normalize(XMVectorSet(rmd_unorm(mt), rmd_unorm(mt), rmd_unorm(mt), 0.05f)) * 1.0f);
			desc.effects[1].child.effects[1].begin_color.w = 0.2f;
			desc.effects[1].child.effects[1].end_color = desc.effects[1].child.effects[1].begin_color;
			desc.effects[1].child.effects[1].end_color.w = 0.f;
			desc.effects[1].child.effects[1].erase_color = desc.effects[1].child.effects[1].end_color;
			desc.effects[1].child.effects[1].scale_back = 0.1f;

			break;
		}
		case 1:
		{
			desc.effects.pop_back();
			desc.effects[0].time = 5.f;
			desc.effects[1].start_time = 5.f;
			desc.effects[1].late = { 50.f, 100.f };
			desc.effects[1].child = desc;
			desc.effects[1].resistivity = 1.f;
			desc.effects[1].has_child = true;
			desc.effects[1].base_speed = { 1.f, 1.f };
			desc.effects[1].child.effects[0].base_speed = { 0.1f, 0.3f };
			desc.effects[1].child.effects[0].late = { 100.f, 100.f };
			desc.effects[1].child.effects[0].time = 3.f;
			desc.effects[1].child.effects[0].speed = { 30.f, 40.f };
			desc.effects[1].child.effects[0].bloom = true;
			desc.effects[1].child.effects[0].angle.x = DirectX::XMConvertToRadians(90.f);
			XMStoreFloat4(&desc.effects[1].child.effects[0].begin_color, XMVector3Normalize(XMVectorSet(rmd_unorm(mt), rmd_unorm(mt), rmd_unorm(mt), 0.05f)) * 1.0f);
			desc.effects[1].child.effects[0].begin_color.w = 0.2f;
			desc.effects[1].child.effects[0].end_color = desc.effects[1].child.effects[0].begin_color;
			desc.effects[1].child.effects[0].erase_color = desc.effects[1].child.effects[0].end_color;
			desc.effects[1].child.effects[0].erase_color.w = 0.f;
			desc.effects[1].child.effects[0].scale_back = 0.1f;
			desc.effects[1].child.effects.pop_back();
			break;
		}
		case 2:
		{
			desc.effects.pop_back();
			desc.effects[0].time = 10.f;
			desc.speed *= 1.5f;
			desc.effects[0].late = { 300.f, 300.f };
			desc.effects[0].spawn_space = { 0.1f, 0.5f };
			desc.effects[1].start_time = 10.f;
			desc.effects[1].alive_time = { 5.f, 6.f };
			desc.effects[1].speed = { 400.f, 600.f };
			desc.effects[1].late = { 20000.f, 20000.f };
			desc.effects[1].scale_back = 0.2f;

			XMStoreFloat4(&desc.effects[1].begin_color, XMVector3Normalize(XMVectorSet(rmd_unorm(mt), rmd_unorm(mt), rmd_unorm(mt), 0.05f)) * 1.f);
			desc.effects[1].begin_color.w = 0.3f;
			desc.effects[1].end_color = desc.effects[1].begin_color;
			desc.effects[1].erase_color = desc.effects[1].end_color;
			desc.effects[1].erase_color.w = 0.f;
			//desc.effects[1].end_color.w = 0.f;
			break;
		}
		default:
		{
			XMStoreFloat4(&desc.effects[1].begin_color, XMVector3Normalize(XMVectorSet(rmd_unorm(mt), rmd_unorm(mt), rmd_unorm(mt), 0.05f)) * 1.f);
			desc.effects[1].begin_color.w = 0.3f;
			desc.effects[1].end_color = desc.effects[1].begin_color;
			desc.effects[1].end_color.w = 0.f;
			desc.effects[1].erase_color = desc.effects[1].end_color;
			desc.effects[2].scale_back = 0.1f;
			XMStoreFloat4(&desc.effects[2].begin_color, XMVector3Normalize(XMVectorSet(rmd_unorm(mt), rmd_unorm(mt), rmd_unorm(mt), 0.05f)) * 1.f);
			desc.effects[2].begin_color.w = 0.3f;
			desc.effects[2].end_color = desc.effects[2].begin_color;
			desc.effects[2].end_color.w = 0.f;
			desc.effects[2].erase_color = desc.effects[2].end_color;
			desc.effects[2].scale_back = 0.1f;
			break;
		}
		}
		return desc;
	}
}
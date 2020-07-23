#pragma once

#include <DirectXMath.h>
#include <vector>

struct Effect
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
	DirectX::XMFLOAT2	scale;					// パーティクル射出時の大きさ
	DirectX::XMFLOAT2	angle;					// パーティクル射出角度
	DirectX::XMFLOAT2	spawn_space;			// パーティクル射出方向へspawn_space分位置をずらします。
	DirectX::XMFLOAT4	color;					// 色
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
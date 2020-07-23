#include "../Hrd/Fireworks.hpp"
#include "../Hrd/Particle.hpp"
#include <Helper/Cast.hpp>
#include <random>
#include <Helper/Debug.hpp>

void Fireworks::Init(const Desc& desc)
{
	std::random_device rd;
	std::mt19937 mt(rd());

	this->desc = desc;

	pos = desc.pos;
	velocity = desc.velocity;

	const auto effect_size = desc.effects.size();
	effects.reserve(effect_size);
	for (size_t i = 0u; i < effect_size; i++)
	{
		auto& data = effects.emplace_back();
		data.effect = desc.effects[i];
		data.update_timer = 0.f;
		data.late_counter = 0.f;
		data.late = std::uniform_real_distribution<float>(data.effect.late.x, data.effect.late.y)(mt);
	}
}

bool Fireworks::Update(float time, std::vector<Particle>* p_particles, const ParticleParent* p_parent)
{
	bool exist = false;
	if (p_particles)
	{
		std::random_device rd;
		std::mt19937 mt(rd());

		using namespace DirectX;
		XMVECTOR xm_pos = XMLoadFloat3(&pos);
		XMVECTOR xm_velocity = XMLoadFloat3(&velocity);
		xm_pos += xm_velocity;
		for (auto& data : effects)
		{
			if (data.effect.start_time > 0.f || data.effect.time > 0.f)
			{
				exist = true;
			}

			float update_time = 0.f;
			if (data.effect.start_time > 0.f)
			{
				data.effect.start_time -= time;
				if (data.effect.start_time <= 0.f)
				{
					data.effect.time -= -data.effect.start_time;
					update_time = -data.effect.start_time;
					xm_pos -= xm_velocity * update_time;
					xm_velocity *= data.effect.start_accel;
					xm_pos += xm_velocity * update_time;
					if (data.effect.time <= 0.f)
					{
						update_time = time - -data.effect.time;
						xm_pos -= xm_velocity * update_time;
						xm_velocity *= data.effect.end_accel;
						xm_pos += xm_velocity * update_time;
					}
				}
			}
			else if (data.effect.start_time <= 0.f)
			{
				if (data.effect.time > 0.f)
				{
					data.effect.time -= time;
					update_time = time;
					if (data.effect.time <= 0.f)
					{
						update_time = time - -data.effect.time;
						xm_pos -= xm_velocity * update_time;
						xm_velocity *= data.effect.end_accel;
						xm_pos += xm_velocity * update_time;
					}
				}
			}
			if (update_time > 0.f)
			{
				data.late_counter -= update_time;
				if (data.late_counter <= 0.f)
				{
					data.late_counter = data.effect.late_update_time - fmodf(-data.late_counter, data.effect.late_update_time);
					data.late = std::uniform_real_distribution<float>(data.effect.late.x, data.effect.late.y)(mt);
				}
				if (XMVector3LengthSq(xm_velocity).m128_f32[0] <= FLT_EPSILON)
					data.Update(xm_pos, XMVectorSet(0.f, 1.f, 0.f, 0.f), update_time, p_particles, p_parent);
				else
					data.Update(xm_pos, xm_velocity, update_time, p_particles, p_parent);
			}
		}
		XMStoreFloat3(&pos, xm_pos);
		XMStoreFloat3(&velocity, xm_velocity);

		/*counter += time;
		if (data.late_update_time < late_counter) late_counter = data.late_update_time;
		late_counter -= time;
		bool explosion = false;
		bool change_flg = false;
		float fly_time = 0.f;
		float explosion_time = 0.f;
		if (fly_counter > 0.f)
		{
			fly_counter -= time;
			fly_time = time;
			if (fly_counter <= 0.f)
			{
				fly_time = time + fly_counter;
				explosion_time = -fly_counter;
				explosion_counter -= -fly_counter;
				explosion = true;
				late_counter = data.late_update_time - explosion_time;
			}
		}
		else if (explosion_counter > 0.f)
		{
			explosion_counter -= time;
			explosion_time = time;
			explosion = true;
		}

		if (late_counter <= 0.f)
		{
			if (late_counter <= 0.f) late_counter = data.late_update_time - fmodf(-late_counter, data.late_update_time);
			
			std::random_device rd;
			std::mt19937 mt(rd());
			std::uniform_int_distribution<int> rdflg(0, 1);
			if (explosion)
			{
				explosion_late = rdflg(mt) == 0 ? data.explosion_desc.late.x : data.explosion_desc.late.y;
			}
			else
			{
				fly_late = rdflg(mt) == 0 ? data.fly_desc.late.x : data.fly_desc.late.y;
			}
		}

		if (fly_time > 0.f)
		{
			using namespace DirectX;
			XMStoreFloat3(&data.pos, XMLoadFloat3(&data.pos) + XMLoadFloat3(&data.velocity) * fly_time);

			EffectUpdate(&fly_update_timer, fly_late, &data.fly_desc, fly_time, p_particles, p_parent);
		}
		if (explosion_time > 0.f)
		{
			EffectUpdate(&explosion_update_timer, explosion_late, &data.explosion_desc, explosion_time, p_particles, p_parent);
		}*/
	}
	return exist;
}

void Fireworks::EffectData::Update(DirectX::CXMVECTOR pos, DirectX::CXMVECTOR velocity,
	float time, std::vector<Particle>* p_particles, const ParticleParent* p_parent)
{
	using namespace DirectX;
	std::random_device rd;
	std::mt19937 mt(rd());

	float spawn_elapsed = 1.0f / late;

	update_timer -= time;
	if (update_timer <= 0.f)
	{
		uint16_t spawn_num = KGL::SCAST<uint16_t>(-update_timer / spawn_elapsed) + 1u;
		float spawn_time_counter = -update_timer;
		update_timer = fmodf(-update_timer, spawn_elapsed);
		XMVECTOR axis = XMVector3Normalize(velocity);
		XMVECTOR right_axis = XMVectorSet(1.f, 0.f, 0.f, 0.f);
		XMVECTOR side_axis;
		if (XMVector3Length(XMVectorSubtract(right_axis, axis)).m128_f32[0] <= FLT_EPSILON)
			side_axis = XMVector3Normalize(XMVector3Cross(axis, XMVectorSet(0.f, 1.f, 0.f, 0.f)));
		else
			side_axis = XMVector3Normalize(XMVector3Cross(axis, right_axis));

		// angleを比率に変換し、０度と180度方向に密集しないよう調整して計算する
		// https://techblog.kayac.com/how-to-distribute-points-randomly-using-high-school-math
		XMFLOAT2 nmangle;
		nmangle.x = effect.angle.x / XM_PI;
		nmangle.y = effect.angle.y / XM_PI;
		std::uniform_real_distribution<float> rmdangle(nmangle.x, nmangle.y);

		std::uniform_real_distribution<float> rmdangle360(0.f, XM_2PI);
		std::uniform_real_distribution<float> rmdspace(effect.spawn_space.x, effect.spawn_space.y);
		std::uniform_real_distribution<float> rmdspeed(effect.speed.x, effect.speed.y);
		std::uniform_real_distribution<float> rmdalivetime(effect.alive_time.x, effect.alive_time.y);
		std::uniform_real_distribution<float> rmdscale(effect.scale.x, effect.scale.y);
		constexpr float radian90f = XMConvertToRadians(90.f);
		XMMATRIX R;
		for (uint16_t i = 0u; i < spawn_num; i++)
		{
			auto& p = p_particles->emplace_back();

			float side_angle = asinf((2.f * rmdangle(mt)) - 1.f) + radian90f;
			//KGLDebugOutPutString(std::to_string(XMConvertToDegrees(side_angle)));
			R = XMMatrixRotationAxis(side_axis, side_angle);
			R *= XMMatrixRotationAxis(axis, rmdangle360(mt));
			XMVECTOR spawn_v = XMVector3Transform(axis, R);
			XMStoreFloat3(&p.position, pos + spawn_v * rmdspace(mt));
			XMStoreFloat3(&p.velocity, spawn_v * rmdspeed(mt));
			p.color = effect.color;
			p.accs = { 0.f, 0.f, 0.f };
			p.exist_time = rmdalivetime(mt);
			p.scale = rmdscale(mt);
			p.mass = 1.f;
			p.Update(spawn_time_counter, p_parent);
			spawn_time_counter -= spawn_elapsed;
		}
	}
}
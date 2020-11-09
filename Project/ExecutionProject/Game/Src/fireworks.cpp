#include "../Hrd/Fireworks.hpp"
#include "../Hrd/Particle.hpp"
#include <Helper/Cast.hpp>
#include <random>
#include <Helper/Debug.hpp>

void Fireworks::Init(const FireworksDesc& desc, float time)
{
	std::random_device rd;
	std::mt19937 mt(rd());

	this->desc = desc;

	this->time = time;
	pos = desc.pos;
	velocity = desc.velocity;
	mass = desc.mass;
	resistivity = desc.resistivity;

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

static constexpr float G = 6.67e-11f;

bool Fireworks::Update(
	float time,
	std::vector<Particle>* p_particles,
	const ParticleParent* p_parent,
	std::vector<Fireworks>* p_fireworks,
	const std::vector<AffectObjects>& affect_objects,
	const std::vector<Fireworks>& affect_fireworks)
{
	if (this->time > 0.f)
	{
		time = this->time;
		this->time = -1.f;
	}
	bool exist = false;
	if (p_particles)
	{
		std::random_device rd;
		std::mt19937 mt(rd());

		using namespace DirectX;
		// Fireworksの位置を更新
		XMVECTOR xm_pos = XMLoadFloat3(&pos);
		XMVECTOR xm_velocity = XMLoadFloat3(&velocity);

		XMVECTOR resultant = XMVectorSet(0.f, 0.f, 0.f, 0.f);
		for (auto& affect_obj : affect_objects)
		{
			XMVECTOR xm_vec = XMLoadFloat3(&affect_obj.pos) - xm_pos;
			float l;
			XMStoreFloat(&l, XMVector3LengthSq(xm_vec));
			// 0になる可能性があるのでEPSILONを最小値にする
			// l = (std::max)(l, FLT_EPSILON);
			float N = (G * mass * affect_obj.mass) / l;
			resultant += XMVector3Normalize(xm_vec) * N;
		}
		for (const auto& affect_fw : affect_fireworks)
		{
			XMVECTOR xm_vec = XMLoadFloat3(&affect_fw.pos) - xm_pos;
			float l;
			XMStoreFloat(&l, XMVector3LengthSq(xm_vec));
			// 0になる可能性があるのでEPSILONを最小値にする
			// l = (std::max)(l, FLT_EPSILON);
			float N = (G * mass * affect_fw.mass) / l;
			resultant += XMVector3Normalize(xm_vec) * N;
		}
		resultant += (-xm_velocity * (p_parent->resistivity * resistivity));
		XMVECTOR xm_accs = (resultant / mass);
		xm_velocity += xm_accs * time;
		xm_pos += xm_velocity * time;
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
					data.Update(xm_pos, XMVectorSet(0.f, 1.f, 0.f, 0.f), update_time, p_particles, p_parent, p_fireworks, affect_objects, affect_fireworks);
				else
					data.Update(xm_pos, xm_velocity, update_time, p_particles, p_parent, p_fireworks, affect_objects, affect_fireworks);
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
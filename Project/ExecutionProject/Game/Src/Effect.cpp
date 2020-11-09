#include "../Hrd/Effects.hpp"
#include "../Hrd/Particle.hpp"
#include "../Hrd/Fireworks.hpp"
#include <Helper/Cast.hpp>
#include <random>

void Effect::Init(const EffectDesc& desc)
{
	effect = desc;
	update_timer = 0.f;
	late_counter = 0.f;
	late = 0.f;
	total_time_count = 0.f;
}

void Effect::Update(DirectX::CXMVECTOR pos, DirectX::CXMVECTOR velocity,
	float time, std::vector<Particle>* p_particles, const ParticleParent* p_parent,
	std::vector<Fireworks>* p_fireworks,
	const std::vector<AffectObjects>& affect_objects,
	const std::vector<Fireworks>& affect_fireworks)
{
	using namespace DirectX;
	std::random_device rd;
	std::mt19937 mt(rd());

	float spawn_elapsed = 1.0f / late;
	update_timer -= time;
	total_time_count += time;
	if (update_timer <= 0.f)
	{
		uint16_t spawn_num = KGL::SCAST<uint16_t>(-update_timer / spawn_elapsed) + 1u;
		float spawn_time_counter = -update_timer;
		float spawn_time_counter_max = spawn_time_counter;
		update_timer += spawn_num * spawn_elapsed;
		XMVECTOR axis = XMVector3Normalize(velocity);
		XMVECTOR right_axis = XMVectorSet(1.f, 0.f, 0.f, 0.f);
		XMVECTOR side_axis;
		XMVECTOR old_pos = pos - (axis * time);
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
		std::uniform_real_distribution<float> rmdbasespeed(effect.base_speed.x, effect.base_speed.y);
		std::uniform_real_distribution<float> rmdalivetime(effect.alive_time.x, effect.alive_time.y);
		std::uniform_real_distribution<float> rmdscale(effect.scale.x, effect.scale.y);
		constexpr float radian90f = XMConvertToRadians(90.f);

		XMMATRIX R;
		if (p_fireworks && effect.has_child)
		{
			const size_t fw_max_size = std::min(p_fireworks->max_size(), p_fireworks->capacity());
			for (uint16_t i = 0u; i < spawn_num; i++)
			{
				if (p_fireworks->size() >= fw_max_size)
					break;

				float side_angle = asinf((2.f * rmdangle(mt)) - 1.f) + radian90f;
				//KGLDebugOutPutString(std::to_string(XMConvertToDegrees(side_angle)));
				R = XMMatrixRotationAxis(side_axis, side_angle);
				R *= XMMatrixRotationAxis(axis, rmdangle360(mt));
				XMVECTOR spawn_v = XMVector3Transform(axis, R);

				auto& desc = effect.child;

				DirectX::XMStoreFloat3(&desc.pos, (old_pos + (-velocity * (spawn_time_counter_max - spawn_time_counter))) + spawn_v * rmdspace(mt));
				DirectX::XMStoreFloat3(&desc.velocity, velocity * rmdbasespeed(mt) + spawn_v * rmdspeed(mt));
				desc.resistivity = effect.resistivity;

				auto& fw = p_fireworks->emplace_back(desc, spawn_time_counter_max - spawn_time_counter);
				spawn_time_counter -= spawn_elapsed;
			}
		}
		else
		{
			const size_t ptc_max_size = std::min(p_particles->max_size(), p_particles->capacity());
			for (uint16_t i = 0u; i < spawn_num; i++)
			{
				if (p_particles->size() >= ptc_max_size)
					break;
				auto& p = p_particles->emplace_back();

				float side_angle = asinf((2.f * rmdangle(mt)) - 1.f) + radian90f;
				//KGLDebugOutPutString(std::to_string(XMConvertToDegrees(side_angle)));
				R = XMMatrixRotationAxis(side_axis, side_angle);
				R *= XMMatrixRotationAxis(axis, rmdangle360(mt));
				XMVECTOR spawn_v = XMVector3Transform(axis, R);
				const float time_space_scale = spawn_time_counter_max - spawn_time_counter;
				DirectX::XMStoreFloat3(&p.position, (old_pos + (-velocity * time_space_scale)) + spawn_v * rmdspace(mt));
				DirectX::XMStoreFloat3(&p.velocity, velocity * rmdbasespeed(mt) + spawn_v * rmdspeed(mt));
				//p.color = (effect.end_color - effect.begin_color);
				CXMVECTOR xm_begin_color = XMLoadFloat4(&effect.begin_color);
				XMVECTOR xm_color = xm_begin_color + (XMLoadFloat4(&effect.end_color) - xm_begin_color) * (std::min(total_time_count, effect.time) / effect.time);
				XMStoreFloat4(&p.color, xm_color);
				
				p.accs = { 0.f, 0.f, 0.f };
				p.exist_time = rmdalivetime(mt);
				XMVECTOR xm_color_speed = (XMLoadFloat4(&effect.erase_color) - xm_color) / p.exist_time;
				XMStoreFloat4(&p.color_speed, xm_color_speed);

				p.scale_width = rmdscale(mt);
				p.scale_front = p.scale_back = p.scale_width;
				p.scale_width *= 2.f;
				p.scale_speed_front = p.scale_front * effect.scale_front;
				p.scale_speed_back = p.scale_back * effect.scale_back;

				p.scale_front_max = p.scale_front + p.scale_speed_front * 100.f;
				p.scale_back_max = p.scale_back + p.scale_speed_back * 100.f;

				p.resistivity = effect.resistivity;
				p.resistivity += effect.scale_resistivity * p.scale_width;
				p.bloom = effect.bloom;
				p.texture_num = effect.id;
				if (effect.bloom)
				{
					p.bloom = effect.bloom;
				}
				//p.scale_speed_back = p.scale_back;
				//p.scale_back * 2.f;
				p.mass = 1.f;
				p.move_length = 0.f;
				p.Update((spawn_time_counter_max - spawn_time_counter), p_parent, affect_objects, affect_fireworks);
				spawn_time_counter -= spawn_elapsed;
			}
		}
	}
}
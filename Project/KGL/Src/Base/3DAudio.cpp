#include <Base/3DAudio.hpp>
#include <Helper/ThrowAssert.hpp>
#include <Helper/Math.hpp>

using namespace KGL;
using namespace BASE;
using namespace AUDIO;

void Manager3D::AddSound(
	const std::shared_ptr<Sound>& sound,
	const Desc& desc,
	float alive_time
)
{
	auto data = std::make_shared<Data>();
	data->sound = sound;
	data->param = desc;
	data->alive_time = alive_time;
	data->start = false;

	m_data.push_back(data);
}

void Manager3D::Update(float elapsed_time, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& front_vec)
{
	for (auto it = m_data.begin(); it != m_data.end();)
	{
		auto& data = (*it);
		auto& param = data->param;
		data->alive_time += elapsed_time;

		const float distance = MATH::Distance(position, param.position);

		// çƒê∂îªíË
		if (!data->start)
		{
			if (distance < data->alive_time * SOUND_SPEED)
			{
				data->sound->Play(0.0f);
				data->start = true;
			}
		}
		else if (!data->sound->IsPlay())
		{
			it = m_data.erase(it);
			continue;
		}

		// âπó çXêV
		{
			const float effect_range_max = param.max_effect_range - param.min_effect_range;
			float effect_range = (std::max)(distance - param.min_effect_range, 0.f);
			effect_range = (std::min)(effect_range, effect_range_max);
			const float volume = param.max_effect_volume * (1.f - (effect_range / effect_range_max));

			//const float rad = atan2f(param.position.z - position.z, param.position.x - position.x);
			//const float angle = 90.f - (DirectX::XMConvertToDegrees(rad) * 0.5f < 0 ? DirectX::XMConvertToDegrees(rad) * 0.5f * -1 : DirectX::XMConvertToDegrees(rad) * 0.5f);
			//data->sound->SetVolume2ch(cosf(DirectX::XMConvertToRadians(angle)) * volume, sinf(DirectX::XMConvertToRadians(angle)) * volume);
		
			using namespace DirectX;

			XMVECTOR target_vec3 = XMLoadFloat3(&param.position) - XMLoadFloat3(&position);
			XMFLOAT3 target_v3, front_v3;
			XMFLOAT2 target_v2, front_v2;
			XMStoreFloat3(&target_v3, target_vec3);
			front_v3 = front_vec;

			// Yç¿ïWÇñ≥éã
			target_v2.x = target_v3.x;
			target_v2.y = target_v3.z;

			front_v2.x = front_v3.x;
			front_v2.y = front_v3.z;

			XMVECTOR target_vec2 = XMVector2Normalize(XMLoadFloat2(&target_v2));
			XMVECTOR front_vec2 = XMVector3Normalize(XMLoadFloat2(&front_v2));

			float angle = KGL::MATH::CalcAngle3(front_vec2, target_vec2);
			float cross_v;
			XMStoreFloat(&cross_v, XMVector2Cross(front_vec2, target_vec2));
			if (cross_v > 0) angle = -angle;

			angle += XMConvertToRadians(45.f);
			data->sound->SetVolume2ch(volume * ((cosf(angle) + 1.f) / 2), volume * ((sinf(angle) + 1.f) / 2));
		}

		it++;
	}
}
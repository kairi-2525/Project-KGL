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

void Manager3D::Update(float elapsed_time, const DirectX::XMFLOAT3& position)
{
	for (auto it = m_data.begin(); it != m_data.end();)
	{
		auto& data = (*it);
		auto& param = data->param;
		data->alive_time += elapsed_time;

		float distance = MATH::Distance(position, param.position);

		// Ä¶”»’è
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

		// ‰¹—ÊXV
		{
			const float effect_range = param.max_effect_range - param.min_effect_range;
			distance = (std::max)(distance - param.min_effect_range, 0.f);
			const float volume = param.max_effect_volume * (1.f - (distance / effect_range));
			data->sound->SetPitch(volume);
			const float rad = atan2f(param.position.z - position.z, param.position.x - position.x);
			const float angle = 90.f - (DirectX::XMConvertToDegrees(rad) * 0.5f < 0 ? DirectX::XMConvertToDegrees(rad) * 0.5f * -1 : DirectX::XMConvertToDegrees(rad) * 0.5f);
			data->sound->SetVolume2ch(cosf(DirectX::XMConvertToRadians(angle)) * volume, sinf(DirectX::XMConvertToRadians(angle)) * volume);
		}

		it++;
	}
}
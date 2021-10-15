#pragma once
#include "Audio.hpp"
#include <DirectXMath.h>

namespace KGL
{
	inline namespace BASE
	{
		namespace AUDIO
		{
			static inline constexpr float SOUND_SPEED = 331.45f;	//������C���������i�ޑ��x

			class Manager3D
			{
			public:
				struct Desc
				{
					DirectX::XMFLOAT3		position;
					DirectX::XMFLOAT3		velocity;

					float					max_effect_range;	// �����艓���ƕ������Ȃ�
					float					min_effect_range;	// ������߂��ƈ��̉���(max_effect_volume)
					float					max_effect_volume;

					float					max_pitch;
					float					min_pitch;
				};
				struct Data
				{
					std::shared_ptr<Sound>	sound;
					Desc					param;
					bool					start;
					float					alive_time;			// �������Ă���̌o�ߎ���
				};
			public:
				static inline const Desc DEFAULT_DESC
				{
					{ 0.f, 0.f, 0.f },
					{ 0.f, 0.f, 0.f },
					1000.f, 1.f,
					1.f,
					1.f, 0.5f
				};
			private:
				std::list<std::shared_ptr<Data>>	m_data;
			public:
				Manager3D() = default;

				void AddSound(
					const std::shared_ptr<Sound>& sound,
					const Desc& desc = DEFAULT_DESC,
					float alive_time = 0.f
					);

				void Update(float elapsed_time, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& front_vec);
			};
		}
	}
}
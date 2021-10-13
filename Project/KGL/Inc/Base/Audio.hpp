#pragma once

#define XAUDIO2_HELPER_FUNCTIONS

#include <xaudio2.h>
#include <filesystem>
#include <memory>
#include <list>
#include <DirectXTKAudio/WAVFileReader.h>

namespace DirectX
{
	struct WAVData;
}

namespace KGL
{
	inline namespace BASE
	{
		namespace AUDIO
		{
			using FILTER_TYPE = XAUDIO2_FILTER_TYPE;
			class Wave;
			class Sound;
		}

		class Audio
		{
		private:
			IXAudio2* m_x_audio2;
			IXAudio2MasteringVoice* m_mastering_voice;
			XAUDIO2_VOICE_DETAILS m_mastering_details{};

			std::list<std::shared_ptr<AUDIO::Sound>> m_sounds;
		private:
			HRESULT Init() noexcept;
			void Clear() noexcept;
		public:
			Audio();
			~Audio();
			IXAudio2* GetXAudio2() const noexcept;
			HRESULT Reset() noexcept;
			void AddSound(const std::shared_ptr<AUDIO::Sound>& sound);
			void Update(float elapsed_time);
			bool IsActive() const noexcept;
			const XAUDIO2_VOICE_DETAILS& GetMasteringDatails() const noexcept;
		};

		namespace AUDIO
		{
			class Sound
			{
			private:
				std::shared_ptr<Wave> m_wave;
				IXAudio2SourceVoice* m_voice;

				bool m_exist;
				bool m_feed_stop;
				bool m_start;
				bool m_filter;
				bool m_db;
				float m_feed_time;
				float m_feed_start_volume;
				float m_feed_target_volume;
				float m_volume;
				std::unique_ptr<float[]> m_balance;

			public:
				struct Desc
				{
					float start_time;
					bool infinity_loop;
					bool wav_loop;
					float loop_start_time;
					float loop_time;
					int loop_count;
					bool use_filter;
					bool db;
					float volume;
				};
				static inline const Desc DEFAULT_DESC = {
					0.f,
					false,
					false,
					0.f,
					0.f,
					0,
					false,
					false,
					1.f
				};
			private:
				void Clear() noexcept;
			public:
				Sound(const std::shared_ptr<Wave>& wave, const Desc& desc = DEFAULT_DESC) noexcept;
				~Sound();
				void Update(float elpased_time);
				void Play(float feed_time = 0.01f);
				void Stop(float feed_time = 0.01f);
				void Pause(float feed_time = 0.01f);
				void SetVolume(float volume, bool db = false);
				float GetVolume(bool db = false);
				void SetVolume2ch(float left_vol, float right_vol, bool db = false);
				void SetPitch(float pitch);
				float GetPitch();

				bool IsAlive() const noexcept;
				bool IsPlay() const noexcept;
			};

			class Wave
			{
			private:
				std::shared_ptr<Audio> m_master;
				std::shared_ptr<std::unique_ptr<uint8_t[]>> m_wave_file;
				DirectX::WAVData m_wave_data;
				std::string m_name;
			public:
				Wave(std::shared_ptr<Audio> audio, std::filesystem::path file);

				std::shared_ptr<Audio> GetMaster() const noexcept;
				const uint8_t* GetFile() const noexcept;
				const DirectX::WAVData& GetData() const noexcept;
				const std::string& GetName() const noexcept;
			};
		}
	}

	inline namespace BASE
	{
		inline IXAudio2* Audio::GetXAudio2() const noexcept
		{
			return m_x_audio2;
		}
		inline const XAUDIO2_VOICE_DETAILS& Audio::GetMasteringDatails() const noexcept
		{
			return m_mastering_details;
		}
		inline bool Audio::IsActive() const noexcept
		{
			return m_x_audio2;
		}

		namespace AUDIO
		{
			inline bool Sound::IsAlive() const noexcept
			{
				return m_voice;
			}
			inline bool Sound::IsPlay() const noexcept
			{
				return m_start;
			}

			inline std::shared_ptr<Audio> Wave::GetMaster() const noexcept
			{
				return m_master;
			}
			inline const uint8_t* Wave::GetFile() const noexcept
			{
				return (*m_wave_file).get();
			}
			inline const DirectX::WAVData& Wave::GetData() const noexcept
			{
				return m_wave_data;
			}
			inline const std::string& Wave::GetName() const noexcept
			{
				return m_name;
			}
		}
	}
}
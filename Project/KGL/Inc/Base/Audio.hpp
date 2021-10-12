#pragma once

#define XAUDIO2_HELPER_FUNCTIONS

#include <xaudio2.h>
#include <filesystem>
#include <memory>
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
		}

		class Audio
		{
		private:
			IXAudio2* m_x_audio2;
			IXAudio2MasteringVoice* m_mastering_voice;
			XAUDIO2_VOICE_DETAILS m_mastering_details{};
		private:
			HRESULT Init() noexcept;
			void Clear() noexcept;
		public:
			Audio();
			~Audio();
			IXAudio2* GetXAudio2() const noexcept;
			HRESULT Reset() noexcept;
			bool IsActive() const noexcept;
			const XAUDIO2_VOICE_DETAILS& GetMasteringDatails() const noexcept;
		};

		namespace AUDIO
		{
			class Sound
			{
			private:
				IXAudio2SourceVoice* voice;

				bool exist;
				bool feed_stop;
				bool start;
				bool filter;
				float feed_time;
				float feed_start_volume;
				float feed_target_volume;
				std::unique_ptr<float[]> balance;
				
				float volume;
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
			public:
				Sound(const Wave& wave, const Desc& desc = DEFAULT_DESC) noexcept;
				~Sound();
				void Update(float elpased_time);
				void Play(float feed_in_time = 0.01f, bool db = false);
			};

			class Wave
			{
			private:
				std::shared_ptr<Audio> m_master;
				std::shared_ptr<std::unique_ptr<uint8_t[]>> m_wave_file;
				DirectX::WAVData m_wave_data;
			public:
				Wave(std::shared_ptr<Audio> audio, std::filesystem::path file);
				std::shared_ptr<Sound> Generate(const Sound::Desc& desc = Sound::DEFAULT_DESC) noexcept;

				std::shared_ptr<Audio> GetMaster() const noexcept;
				const uint8_t* GetFile() const noexcept;
				const DirectX::WAVData& GetData() const noexcept;
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
		}
	}
}
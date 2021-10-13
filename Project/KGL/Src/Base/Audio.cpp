#include <Base/Audio.hpp>
#include <Helper/ThrowAssert.hpp>

using namespace KGL;
using namespace BASE;
using namespace AUDIO;

Audio::Audio() : 
	m_x_audio2(nullptr),
	m_mastering_voice(nullptr)
{
	HRESULT hr = S_OK;
	hr = Reset();
	assert(SUCCEEDED(hr) && "XAudio2の初期化に失敗");
}

Audio::~Audio()
{
	Clear();
}

HRESULT Audio::Reset() noexcept
{
	if (IsActive()) Clear();
	return Init();
}

HRESULT Audio::Init() noexcept
{
	HRESULT hr = S_OK;
	hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
	assert(SUCCEEDED(hr) && "XAudio2の初期化CoInitializeExに失敗");

	UINT32 flags = 0;
#if (_WIN32_WINNT < _WIN32_WINNT_WIN8) && defined(_DEBUG)
	flags |= XAUDIO2_DEBUG_ENGINE;
#endif

	hr = XAudio2Create(&m_x_audio2, flags);
	RCHECK_HR(hr, "XAudio2の初期化に失敗");

	hr = m_x_audio2->CreateMasteringVoice(&m_mastering_voice);
	if (FAILED(hr))	//おそらく再生可能デバイスが存在しない場合。 
	{
		m_x_audio2->Release();
		m_x_audio2 = nullptr;
		m_mastering_voice = nullptr;
		CoUninitialize();
		return S_FALSE; // S_FALSEはエラーにならないが使わない可能性もあるのでこのまま返す
	}
	m_mastering_voice->GetVoiceDetails(&m_mastering_details);

	return hr;
}

void Audio::Clear() noexcept
{
	if (m_x_audio2)
	{
		m_sounds.clear();

		m_mastering_voice->DestroyVoice();
		m_x_audio2->Release();
		CoUninitialize();
		m_x_audio2 = nullptr;
		m_mastering_voice = nullptr;
	}
}

void Audio::AddSound(const std::shared_ptr<Sound>& sound)
{
	m_sounds.push_back(sound);
}

void Audio::Update(float elapsed_time)
{
	if (m_x_audio2)
	{
		for (auto itr = m_sounds.begin(); itr != m_sounds.end();)
		{
			(*itr)->Update(elapsed_time);
			if (!(*itr)->IsAlive())
			{
				itr = m_sounds.erase(itr);
				continue;
			}
			itr++;
		}
	}
}

Sound::Sound(const std::shared_ptr<Wave>& wave, const Desc& desc) noexcept
{
	if (!wave) return;
	m_wave = wave;
	auto master = m_wave->GetMaster();
	if (!master->IsActive()) return;
	HRESULT hr = S_OK;

	m_exist = true;
	m_feed_stop = false;
	m_start = false;
	m_filter = desc.use_filter;
	m_feed_time = -1.f;
	m_db = desc.db;
	m_volume = m_db ? XAudio2DecibelsToAmplitudeRatio(desc.volume) : desc.volume;
	m_feed_start_volume = 0;
	m_feed_target_volume = 0;

	const auto& wav_data = m_wave->GetData();
	
	hr = master->GetXAudio2()->CreateSourceVoice(&m_voice, wav_data.wfx,
		m_filter ? XAUDIO2_VOICE_USEFILTER : 0U, XAUDIO2_MAX_FREQ_RATIO);
	assert(SUCCEEDED(hr) && "Soundの初期化に失敗");

	XAUDIO2_BUFFER buffer = {};
	buffer.pAudioData = wav_data.startAudio;
	buffer.Flags = XAUDIO2_END_OF_STREAM;
	buffer.AudioBytes = wav_data.audioBytes;
	buffer.PlayBegin = (UINT)(desc.start_time * wav_data.wfx->nSamplesPerSec);

	if (desc.infinity_loop || desc.loop_count > 0)
	{
		if (desc.wav_loop)
		{
			buffer.LoopBegin = wav_data.loopStart;
			buffer.LoopLength = wav_data.loopLength;
		}
		else
		{
			buffer.LoopBegin = (UINT)(desc.loop_start_time * wav_data.wfx->nSamplesPerSec);
			buffer.LoopLength = (UINT)(desc.loop_time * wav_data.wfx->nSamplesPerSec);
		}
		if (desc.infinity_loop) buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
		else buffer.LoopCount = desc.loop_count;
	}

	if (wav_data.seek)
	{
		m_voice->DestroyVoice();
		m_voice = nullptr;
		assert(!"xWMA or XMA2が読み込まれました");
		return;
	}
	if (FAILED(hr = m_voice->SubmitSourceBuffer(&buffer)))
	{
		m_voice->DestroyVoice();
		m_voice = nullptr;
		assert(!"SubmitSourceBufferでエラー");
		return;
	}
	const auto& mastering_details = master->GetMasteringDatails();
	m_balance.reset(new float[mastering_details.InputChannels]);
	for (UINT i = 0; i < mastering_details.InputChannels; i++)
		m_balance[i] = 0.707f;
}

Sound::~Sound()
{
	Clear();
}

void Sound::Clear() noexcept
{
	if (m_voice)
	{
		m_voice->DestroyVoice();
		m_voice = nullptr;
	}
}

void Sound::Update(float elpased_time)
{
	if (m_voice)
	{
		XAUDIO2_VOICE_STATE state = {};
		m_voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);

		if (state.BuffersQueued == 0U)
		{
			return Clear();
		}
		
		if (m_feed_time >= 0 && m_start)
		{
			float volume = 0;
			m_voice->GetVolume(&volume);
			const float get_volume = volume;
			if (m_feed_start_volume < m_feed_target_volume)
			{
				const float difference = m_feed_target_volume - m_feed_start_volume;
				volume = std::min<float>(volume + (difference / m_feed_time) * elpased_time, m_feed_target_volume);
			}
			else if (m_feed_start_volume > m_feed_target_volume)
			{
				const float difference = m_feed_start_volume - m_feed_target_volume;
				volume = std::max<float>(volume - (difference / m_feed_time) * elpased_time, m_feed_target_volume);
			}

			if (get_volume == m_feed_target_volume)
			{
				m_feed_time = -1;
				if (m_feed_stop)
				{
					m_feed_stop = false;
					m_voice->Stop(XAUDIO2_PLAY_TAILS);
					m_start = false;
				}
				if (!m_exist)
				{
					return Clear();
				}
			}

			if (FAILED(m_voice->SetVolume(volume)))
				assert(!"voice->SetVolumeでエラー");
		}
	}
}

void Sound::Play(float feed_time)
{
	if (feed_time > 0 && m_volume > 0)
	{
		m_voice->SetVolume(0.f);
		m_feed_start_volume = 0;
		m_feed_time = feed_time;
		m_feed_target_volume = m_volume;
	}
	else m_voice->SetVolume(m_volume);
	m_voice->Start();
	m_start = true;
}

void Sound::Stop(float feed_time)
{
	if (m_voice)
	{
		if (m_start)
		{
			m_voice->GetVolume(&m_feed_start_volume);
			m_feed_time = feed_time;
			m_feed_target_volume = 0;
			m_exist = false;
		}
		else
		{
			Clear();
		}
	}
}

void Sound::Pause(float feed_time)
{
	if (m_voice)
	{
		m_voice->GetVolume(&m_feed_start_volume);
		m_feed_time = feed_time;
		m_feed_target_volume = 0;
		m_feed_stop = true;
	}
}

void Sound::SetVolume(float volume, bool db)
{
	if (m_voice)
	{
		if (m_feed_time < 0)
		{
			HRESULT hr = S_OK;
			hr = m_voice->SetVolume(db ? XAudio2DecibelsToAmplitudeRatio(volume) : volume);
			if (FAILED(hr))
			{
				assert(!"sound.voice->SetVolumeでエラー(Audio::SetVolume)");
			}
		}
	}
}

float Sound::GetVolume(bool db)
{
	float volume = 0.f;
	if (m_voice)
	{
		m_voice->GetVolume(&volume);
		return db ? XAudio2AmplitudeRatioToDecibels(volume) : volume;
	}
	return volume;
}

void Sound::SetVolume2ch(float left_vol, float right_vol, bool db)
{
	if (m_voice)
	{
		HRESULT hr = S_OK;
		if (db)
		{
			left_vol = XAudio2DecibelsToAmplitudeRatio(left_vol);
			right_vol = XAudio2DecibelsToAmplitudeRatio(right_vol);
		}

		const auto& mastering_details = m_wave->GetMaster()->GetMasteringDatails();

		std::vector<float> volumes(mastering_details.InputChannels);
		for (UINT i = 0; i < mastering_details.InputChannels; ++i) {
			switch (i)
			{
				case 0: volumes[i] = left_vol; break;
				case 1: volumes[i] = right_vol; break;
				default: volumes[i] = 0; break;
			}
		}

		XAUDIO2_VOICE_DETAILS details;
		m_voice->GetVoiceDetails(&details);
		if (details.InputChannels > mastering_details.InputChannels)
		{
			assert(!"入力が出力より大きい(Audio::SetVolume2ch)");
			return;
		}
		
		if (FAILED(hr))
		{
			assert(!"sound.voice->SetChannelVolumesでエラー(Audio::SetVolume2ch)");
			return;
		}

		hr = m_voice->SetChannelVolumes(details.InputChannels, volumes.data());
		//hr = m_voice->SetOutputMatrix(nullptr, details.InputChannels, mastering_details.InputChannels, volumes.data());
		if (FAILED(hr))
		{
			assert(!"sound.voice->SetOutputMatrixでエラー(Audio::SetVolume2ch)");
			return;
		}
	}
}

void Sound::SetPitch(float pitch)
{
	if (m_voice)
	{
		m_voice->SetFrequencyRatio(pitch);
	}
}

float Sound::GetPitch()
{
	float pitch = 0.f;
	if (m_voice)
	{
		m_voice->GetFrequencyRatio(&pitch);
	}
	return pitch;
}

Wave::Wave(std::shared_ptr<Audio> audio, std::filesystem::path file)
{
	if (!audio) return;
	if (!audio->IsActive()) return;

	m_master = audio;
	m_wave_file = std::make_shared<std::unique_ptr<uint8_t[]>>();
	m_name = file.stem().string();
	HRESULT hr = DirectX::LoadWAVAudioFromFileEx(file.c_str(), *m_wave_file, m_wave_data);
	if (FAILED(hr))
	{
		throw std::runtime_error("[ " + file.string() + " ] の読み込みに失敗。");
	}
}
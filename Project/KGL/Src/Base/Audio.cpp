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
	//hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
	//assert(SUCCEEDED(hr) && "XAudio2の初期化に失敗");

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
		// CoUninitialize();
		return S_FALSE; // S_FALSEはエラーにならないが使わない可能性もあるのでこのまま返す
	}

	return hr;
}

void Audio::Clear() noexcept
{
	if (m_x_audio2)
	{
		m_mastering_voice->DestroyVoice();
		m_x_audio2->Release();
		// CoUninitialize();
		m_x_audio2 = nullptr;
		m_mastering_voice = nullptr;
	}
}

Sound::Sound(const Wave& wave, const Desc& desc) noexcept
{
	auto master = wave.GetMaster();
	if (!master->IsActive()) return;
	HRESULT hr = S_OK;

	exist = true;
	feed_stop = false;
	start = false;
	filter = desc.use_filter;
	feed_time = -1.f;
	feed_start_volume = 1.f;
	feed_target_volume = 1.f;
	volume = desc.volume;

	const auto& wav_data = wave.GetData();
	
	hr = master->GetXAudio2()->CreateSourceVoice(&voice, wav_data.wfx,
		filter ? XAUDIO2_VOICE_USEFILTER : 0U, XAUDIO2_MAX_FREQ_RATIO);
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
		voice->DestroyVoice();
		voice = nullptr;
		assert(!"xWMA or XMA2が読み込まれました");
		return;
	}
	if (FAILED(hr = voice->SubmitSourceBuffer(&buffer)))
	{
		voice->DestroyVoice();
		voice = nullptr;
		assert(!"SubmitSourceBufferでエラー");
		return;
	}
	const auto& mastering_details = master->GetMasteringDatails();
	balance.reset(new float[mastering_details.InputChannels]);
	for (UINT i = 0; i < mastering_details.InputChannels; i++)
		balance[i] = 0.707f;
}

Sound::~Sound()
{
	voice->DestroyVoice();
	voice = nullptr;
}

void Sound::Play(float feed_in_time, bool db)
{
	if (db) volume = XAudio2DecibelsToAmplitudeRatio(volume);
	if (feed_in_time > 0 && volume > 0)
	{
		voice->SetVolume(0.f);
		feed_start_volume = 0;
		feed_time = feed_in_time;
		feed_target_volume = volume;
	}
	else voice->SetVolume(volume);
	voice->Start();
	start = true;
}

Wave::Wave(std::shared_ptr<Audio> audio, std::filesystem::path file)
{
	if (!audio) return;
	if (!audio->IsActive()) return;

	m_master = audio;
	m_wave_file = std::make_shared<std::unique_ptr<uint8_t[]>>();

	HRESULT hr = DirectX::LoadWAVAudioFromFileEx(file.c_str(), *m_wave_file, m_wave_data);
	if (FAILED(hr))
	{
		throw std::runtime_error("[ " + file.string() + " ] の読み込みに失敗。");
	}
}

std::shared_ptr<Sound> Wave::Generate(const Sound::Desc& desc) noexcept
{
	std::shared_ptr<Sound> sound = std::make_shared<Sound>(*this, desc);

	return sound;
}
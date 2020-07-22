#pragma once

#include <mutex>
#include <thread>
#include <memory>
#include <filesystem>
#include <string>

using UINT = unsigned int;

namespace KGL
{
	inline namespace HELPER
	{
		//JIS_X_0208
		extern const unsigned short glyphRangesJapanese[];

		//�t�@�C���T�C�Y��byte�ŕԂ�(�t�@�C����������Ȃ��ꍇfalse��Ԃ�)
		extern bool GetFileSize(const std::filesystem::path& path, size_t* out_size);
		//�t�@�C���T�C�Y��byte�ŕԂ�(�t�@�C����������Ȃ��ꍇruntime_error��throw����)
		extern size_t GetFileSize(const std::filesystem::path& path);
		//byte��P�ʒ�������STR�ŕԂ�(get_size:-1 = �����_�̐؂�̂Ă��s��Ȃ�)
		extern std::string GetSizeToStr(size_t byte, int get_size = -1);
		//GUID��String�`���ɕϊ�
		extern std::string CreateGUIDToStr(const _GUID& guid);
	}
}
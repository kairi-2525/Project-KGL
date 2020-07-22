#include <Helper/Other.hpp>
#include <assert.h>
#include <fstream>

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#pragma comment(lib, "Rpcrt4.lib")

using namespace KGL;

//�t�@�C���T�C�Y�̎擾
static inline size_t GetFileSize(std::ifstream* p_ifs)
{
	assert(p_ifs && "GetFileSize��p_ifs��nullptr");
	p_ifs->seekg(0, std::ios::end);
	auto eofpos = p_ifs->tellg();
	p_ifs->clear();
	p_ifs->seekg(0, std::ios::beg);
	auto begpos = p_ifs->tellg();
	return eofpos - begpos;
}

//�t�@�C���T�C�Y��byte�ŕԂ�(�t�@�C����������Ȃ��ꍇfalse��Ԃ�)
extern inline bool HELPER::GetFileSize(const std::filesystem::path& path, size_t* out_size)
{
	assert(out_size && "GetFileSize�̏o�͂�nullptr");
	std::ifstream ifs(path, std::ios::binary);
	if (ifs)
	{
		*out_size = ::GetFileSize(&ifs);
		ifs.close();
		return true;
	}
	return false;
}

//�t�@�C���T�C�Y��byte�ŕԂ�
extern inline size_t HELPER::GetFileSize(const std::filesystem::path& path)
{
	std::ifstream ifs(path, std::ios::binary);
	if (!ifs) throw std::runtime_error("\"" + path.string() + "\"�̓ǂݍ��݂Ɏ��s!");
	size_t size = ::GetFileSize(&ifs);
	ifs.close();
	return size;
}

//�����_�̐؂�̂Ă��s��
static inline std::string StrNumberReSize(const std::string& str, int get_size)
{
	auto i = str.rfind('.');
	if (i == std::string::npos || get_size < 0) return str;
	i += get_size;
	if (get_size > 0) i++;
	std::string ret_str = str;
	ret_str.erase(ret_str.begin() + i, ret_str.end());
	return ret_str;
}

//�o�C�g��P�ʒ�������STR�ŕԂ�
extern inline std::string HELPER::GetSizeToStr(size_t byte, int get_size)
{
	std::string a;
	float bytef = static_cast<float>(byte);
	if (bytef / 1000.f >= 1.f)
	{
		bytef /= 1000.f;
		if (bytef / 1000.f >= 1.f)
		{
			bytef /= 1000.f;
			if (bytef / 1000.f >= 1.f)
			{
				bytef /= 1000.f;
				if (bytef / 1000.f >= 1.f)
				{
					bytef /= 1000.f;
					return StrNumberReSize(std::to_string(bytef), get_size) + "TB";
				}
				else return StrNumberReSize(std::to_string(bytef), get_size) + "GB";
			}
			else return StrNumberReSize(std::to_string(bytef), get_size) + "MB";
		}
		else return StrNumberReSize(std::to_string(bytef), get_size) + "KB";
	}
	else return std::to_string(byte) + "Byte";
}

extern inline std::string HELPER::CreateGUIDToStr(const _GUID& guid)
{
	CHAR* uuid_str = NULL;
	UuidToStringA(&guid, (RPC_CSTR*)&uuid_str);
	return uuid_str;
}
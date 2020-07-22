#include <Helper/Other.hpp>
#include <assert.h>
#include <fstream>

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#pragma comment(lib, "Rpcrt4.lib")

using namespace KGL;

//ファイルサイズの取得
static inline size_t GetFileSize(std::ifstream* p_ifs)
{
	assert(p_ifs && "GetFileSizeのp_ifsがnullptr");
	p_ifs->seekg(0, std::ios::end);
	auto eofpos = p_ifs->tellg();
	p_ifs->clear();
	p_ifs->seekg(0, std::ios::beg);
	auto begpos = p_ifs->tellg();
	return eofpos - begpos;
}

//ファイルサイズをbyteで返す(ファイルが見つからない場合falseを返す)
extern inline bool HELPER::GetFileSize(const std::filesystem::path& path, size_t* out_size)
{
	assert(out_size && "GetFileSizeの出力がnullptr");
	std::ifstream ifs(path, std::ios::binary);
	if (ifs)
	{
		*out_size = ::GetFileSize(&ifs);
		ifs.close();
		return true;
	}
	return false;
}

//ファイルサイズをbyteで返す
extern inline size_t HELPER::GetFileSize(const std::filesystem::path& path)
{
	std::ifstream ifs(path, std::ios::binary);
	if (!ifs) throw std::runtime_error("\"" + path.string() + "\"の読み込みに失敗!");
	size_t size = ::GetFileSize(&ifs);
	ifs.close();
	return size;
}

//小数点の切り捨てを行う
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

//バイトを単位調整してSTRで返す
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
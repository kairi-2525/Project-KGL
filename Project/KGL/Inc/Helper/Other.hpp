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

		//ファイルサイズをbyteで返す(ファイルが見つからない場合falseを返す)
		extern bool GetFileSize(const std::filesystem::path& path, size_t* out_size);
		//ファイルサイズをbyteで返す(ファイルが見つからない場合runtime_errorをthrowする)
		extern size_t GetFileSize(const std::filesystem::path& path);
		//byteを単位調整してSTRで返す(get_size:-1 = 小数点の切り捨てを行わない)
		extern std::string GetSizeToStr(size_t byte, int get_size = -1);
		//GUIDをString形式に変換
		extern std::string CreateGUIDToStr(const _GUID& guid);
	}
}
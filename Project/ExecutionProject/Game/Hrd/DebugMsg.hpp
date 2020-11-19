#pragma once

#include <chrono>
#include <string>
#include <DirectXMath.h>
#include <list>
#include <memory>
#include <vector>
#include <mutex>

class DebugMsgMgr
{
public:
	static inline const DirectX::XMUINT3 CL_MSG		= { 255u, 255u, 255u };
	static inline const DirectX::XMUINT3 CL_SUCCESS	= { 000u, 255u, 000u };
	static inline const DirectX::XMUINT3 CL_WARNING	= { 255u, 255u, 000u };
	static inline const DirectX::XMUINT3 CL_ERROR	= { 255u, 000u, 000u };
private:
	struct DebugMsg
	{
		std::string								tag;
		std::string								msg;
		DirectX::XMUINT3						color;
		std::string								time_msg;
		std::wstring							debug_msg;
		std::chrono::system_clock::time_point	time_point;
	};
	struct Tag
	{
		const std::string						name;
		bool									search;
	};
	struct Color
	{
		const DirectX::XMUINT3					color;
		bool									search;
	};
private:
	std::mutex									msg_mutex;
	std::list<std::shared_ptr<const DebugMsg>>	msg_list;
public:
	std::list<Tag>								tag_list;
	std::list<Color>							color_list;
	std::string									search_text;
public:
	void AddMessage(const std::string& msg, const DirectX::XMUINT3& color = CL_MSG, const std::string& tag = "NONE");
	void UpdateGui();
	void Clear();
	const std::list<Tag>& GetTagList() const { return tag_list; }
};
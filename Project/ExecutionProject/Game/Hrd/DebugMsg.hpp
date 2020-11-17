#pragma once

#include <chrono>
#include <string>
#include <DirectXMath.h>
#include <list>
#include <memory>
#include <vector>

class DebugMsgMgr
{
public:
	static inline const DirectX::XMFLOAT4 CL_MSG		= { 1.f, 1.f, 1.f, 1.f };
	static inline const DirectX::XMFLOAT4 CL_SUCCESS	= { 0.f, 1.f, 0.f, 1.f };
	static inline const DirectX::XMFLOAT4 CL_WARNING	= { 1.f, 1.f, 0.f, 1.f };
	static inline const DirectX::XMFLOAT4 CL_ERROR		= { 1.f, 0.f, 0.f, 1.f };
private:
	struct DebugMsg
	{
		std::string								msg;
		DirectX::XMFLOAT4						color;
		std::string								time_msg;
		std::wstring							debug_msg;
		std::chrono::system_clock::time_point	time_point;
		std::vector<bool>						multi_byte_flags;
	};
private:
	std::list<std::shared_ptr<const DebugMsg>>	msg_list;
public:
	void AddMessage(const std::string& msg, const DirectX::XMFLOAT4& color = CL_MSG);
	void UpdateGui();
	void Clear();
};
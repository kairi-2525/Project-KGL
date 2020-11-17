#include "../../Hrd/DebugMsg.hpp"
#include <ctime>
#include <iomanip>
#include <sstream>
#include <imgui.h>
#include <filesystem>
#include <Helper/Cast.hpp>
#include <mbstring.h>

void DebugMsgMgr::AddMessage(const std::string& msg, const DirectX::XMFLOAT4& color)
{
	auto data = std::make_shared<DebugMsg>();
	data->time_point = std::chrono::system_clock::now();
	
	std::time_t t = std::chrono::system_clock::to_time_t(data->time_point);
	std::tm lt;
	errno_t error;
	error = localtime_s(&lt, &t);
	if (error != 0)
	{
		return;
	}
	std::stringstream ss;

	ss.imbue(std::locale("ja_JP.utf8"));
	ss << std::put_time(&lt, "%H:%M:%S");

	data->time_msg = ss.str();
	data->msg = msg;
	data->color = color;

	auto nw_msg = "[" + data->time_msg + "] : " + data->msg;
	data->debug_msg = std::filesystem::path(nw_msg);

	const size_t msg_lenght = data->debug_msg.length();
	data->multi_byte_flags.resize(msg_lenght);
	auto& flags = data->multi_byte_flags;
	size_t i = 0u;
	for (size_t wi = 0u; i < msg_lenght; wi++)
	{
		flags[wi] = _mbclen((const unsigned char*)&nw_msg[i]) != 1;
		if (flags[wi])
		{
			i++;
		}
		i++;
	}

	msg_list.push_front(data);
}

void DebugMsgMgr::UpdateGui()
{
	const auto window_width = ImGui::GetWindowSize().x;
	
	float one_size = ImGui::CalcTextSize("a").x;
	float wide_size = ImGui::CalcTextSize(u8"あ").x;

	for (const auto& data : msg_list)
	{
		auto msg = std::filesystem::path(data->debug_msg).wstring();

		// サイズがなくなるまで
		while (msg.length() > 0)
		{
			// 一文字のサイズ
			const float one_size = ImGui::CalcTextSize(std::filesystem::path(msg).u8string().c_str()).x / msg.length();
			// ウィンドウに収まる文字数
			const size_t window_text_count = SCAST<size_t>(window_width / one_size);

			auto draw_msg = msg;
			if (draw_msg.length() > window_text_count)
			{
				draw_msg = draw_msg.substr(0, window_text_count);
			}
			ImGui::Text(std::filesystem::path(draw_msg).u8string().c_str());

			msg = msg.substr((std::min)(window_text_count, msg.length()));
		}
	}
}

void DebugMsgMgr::Clear()
{
	msg_list.clear();
}
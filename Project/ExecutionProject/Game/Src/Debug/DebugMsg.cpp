#include "../../Hrd/DebugMsg.hpp"
#include <Helper/Convert.hpp>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <imgui.h>
#include <filesystem>
#include <Helper/Cast.hpp>
#include <mbstring.h>
#include <algorithm>

void DebugMsgMgr::AddMessage(const std::string& msg, const DirectX::XMUINT3& color, const std::string& tag)
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
	data->tag = tag;

	auto nw_msg = "[" + data->time_msg + "] : " + data->msg;
	data->debug_msg = std::filesystem::path(nw_msg).wstring();

	std::lock_guard<std::mutex> lock(msg_mutex);
	// タグが既存のものでない場合Listに追加する
	if (tag_list.end() == std::find_if(tag_list.begin(), tag_list.end(), [tag](const Tag& x) { return x.name == tag; }))
		tag_list.push_front(Tag{ tag, true });

	// カラーが既存のものでない場合にListに追加する
	if (color_list.end() == std::find_if(color_list.begin(), color_list.end(),
		[color](const Color& c)
		{
			return c.color.x == color.x && c.color.y == color.y && c.color.z == color.z;
		}
	)) {
		color_list.push_front(Color{ color, true });
	}

	msg_list.push_front(data);
}

// 空白文字を削除
static std::string StringTrim(const std::string& string, const char* trimCharacterList = " \t\v\r\n")
{
	std::string result;

	// 左側からトリムする文字以外が見つかる位置を検索します。
	std::string::size_type left = string.find_first_not_of(trimCharacterList);

	if (left != std::string::npos)
	{
		// 左側からトリムする文字以外が見つかった場合は、同じように右側からも検索します。
		std::string::size_type right = string.find_last_not_of(trimCharacterList);

		// 戻り値を決定します。ここでは右側から検索しても、トリムする文字以外が必ず存在するので判定不要です。
		result = string.substr(left, right - left + 1);

	}
	return result;

}

void DebugMsgMgr::UpdateGui()
{
	using std::filesystem::path;

	size_t gui_idx = 0u;

	constexpr size_t text_size = 32u;
	if (search_text.capacity() < text_size)
		search_text.reserve(text_size);
	search_text.resize(text_size);
	ImGui::InputText(("##" + std::to_string(gui_idx++)).c_str(), search_text.data(), text_size);
	search_text.erase(remove(search_text.begin(), search_text.end(), '\0'), search_text.end());

	ImGui::SameLine();

	// TAG
	std::string tag_popup_id = "Tags##" + std::to_string(gui_idx++);
	if (ImGui::Button(u8"Tag"))
	{
		ImGui::OpenPopup(tag_popup_id.c_str());
	}

	if (ImGui::BeginPopup(tag_popup_id.c_str()))
	{
		for (auto& tag : tag_list)
		{
			if (ImGui::RadioButton(KGL::MultiToUtf8(tag.name).c_str(), tag.search))
			{
				tag.search = !tag.search;
			}
		}
		ImGui::EndPopup();
	}

	ImGui::SameLine();

	// COLOR
	std::string col_popup_id = "Color##" + std::to_string(gui_idx++);
	if (ImGui::Button(u8"Color"))
	{
		ImGui::OpenPopup(col_popup_id.c_str());
	}

	if (ImGui::BeginPopup(col_popup_id.c_str()))
	{
		for (auto& col : color_list)
		{
			if (ImGui::RadioButton(("##" + std::to_string(gui_idx++)).c_str(), col.search))
			{
				col.search = !col.search;
			}
			ImGui::SameLine();
			ImVec4 col_color = { SCAST<float>(col.color.x) / 255, SCAST<float>(col.color.y) / 255, SCAST<float>(col.color.z) / 255, 1.f };
			if (ImGui::ColorButton(("##" + std::to_string(gui_idx++)).c_str(), col_color))
			{
				col.search = !col.search;
			}
		}
		ImGui::EndPopup();
	}

	if (ImGui::BeginChild("DebugMsgLog Window", { -1.f, -1.f }, true, ImGuiWindowFlags_NoTitleBar))
	{
		const auto window_width = ImGui::GetWindowSize().x * 0.9f;

		std::lock_guard<std::mutex> lock(msg_mutex);
		for (const auto& data : msg_list)
		{
			auto msg = data->debug_msg;

			// search text を含まないメッセージを除外
			if (!search_text.empty())
			{
				auto pos = msg.find(KGL::Utf8ToWide(search_text));
				if (pos == std::string::npos)
					continue;
			}
			// tag_list に含まれない メッセージを除外
			{
				bool continue_flg = false;
				for (const auto& tag : tag_list)
				{
					if (!tag.search)
					{
						if (tag.name == data->tag)
						{
							continue_flg = true;
							break;
						}
					}
				}
				if (continue_flg) continue;
			}

			// color_list に含まれない メッセージを除外
			{
				bool continue_flg = false;
				for (const auto& col : color_list)
				{
					if (!col.search)
					{
						if (col.color.x == data->color.x && col.color.y == data->color.y && col.color.z == data->color.z)
						{
							continue_flg = true;
							break;
						}
					}
				}
				if (continue_flg) continue;
			}

			const ImVec4 color = ImVec4(SCAST<float>(data->color.x) / 255, SCAST<float>(data->color.y) / 255, SCAST<float>(data->color.z) / 255, 1.f);
			// テキストを改行する
			while (!msg.empty())
			{
				float text_length = ImGui::CalcTextSize(path(msg).u8string().c_str()).x;

				auto msg_length = msg.length();
				msg_length = (std::min)(SCAST<size_t>(msg_length * (window_width / text_length)), msg_length);

				ImGui::TextColored(color, path(msg.substr(0, msg_length)).u8string().c_str());
				msg = msg.substr(msg_length);
			}
		}
	}
	ImGui::EndChild();
}

void DebugMsgMgr::Clear()
{
	msg_list.clear();
}
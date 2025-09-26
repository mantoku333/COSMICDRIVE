#define _CRT_SECURE_NO_WARNINGS

#include "Debug.h"
#include "GUI.h"
#include "DirectX11.h"
#include "App.h"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

std::vector<sf::debug::Debug::LogData> sf::debug::Debug::logDatas;
std::queue<sf::debug::Debug::LineData> sf::debug::Debug::lineDatas;
std::mutex sf::debug::Debug::mtx;
sf::file::TextFile sf::debug::Debug::textFile("log\\log.txt");

void sf::debug::Debug::WriteLogFile()
{
	textFile.Clean();
}

void sf::debug::Debug::Log(std::string str)
{
	LogData data;
	data.logKind = LogData::LogKind::Normal;
	data.log = str;
	data.logu8 = ConvertToUTF8(str);
	data.timeText = GetTime();

	AddLog(data);
}

void sf::debug::Debug::Log(const Vector3& v)
{
	Log(
		"X:" + std::to_string(v.x) + "　"
		"Y:" + std::to_string(v.y) + "　"
		"Z:" + std::to_string(v.z)
	);
}

void sf::debug::Debug::LogSafety(std::string str)
{
	LogData data;
	data.logKind = LogData::LogKind::Safety;
	data.log = str;
	data.logu8 = ConvertToUTF8(str);
	data.timeText = GetTime();

	AddLog(data);
}

void sf::debug::Debug::LogWarning(std::string str)
{
	LogData data;
	data.logKind = LogData::LogKind::Warning;
	data.log = str;
	data.logu8 = ConvertToUTF8(str);
	data.timeText = GetTime();

	AddLog(data);
}

void sf::debug::Debug::LogError(std::string str)
{
	LogData data;
	data.logKind = LogData::LogKind::Error;
	data.log = str;
	data.logu8 = ConvertToUTF8(str);
	data.timeText = GetTime();

	AddLog(data);
}

void sf::debug::Debug::LogEngine(std::string str)
{
	LogData data;
	data.logKind = LogData::LogKind::Engine;
	data.log = str;
	data.logu8 = ConvertToUTF8(str);
	data.timeText = GetTime();

	AddLog(data);
}

void sf::debug::Debug::DrawLine(Vector3 p1, Vector3 p2, DirectX::XMFLOAT4 color)
{
#if !DEBUG_LINE
	return;
#endif
	//スレッド同期区間
	{
		std::lock_guard<std::mutex> lock(mtx);

		LineData data;
		data.p1 = DirectX::XMFLOAT4(p1.x, p1.y, p1.z, 0);
		data.p2 = DirectX::XMFLOAT4(p2.x, p2.y, p2.z, 0);
		data.color = color;
		lineDatas.push(data);
	}
}

void sf::debug::Debug::DrawLog()
{
#if DEBUG_LINE
	if (lineDatas.empty())
#endif
		return;

	dx::DirectX11* dx11 = dx::DirectX11::Instance();

	ID3D11RenderTargetView* rtv[]
	{
		dx11->rb3d.Get().GetBaseColorRTV().GetRTV(),
	};

	dx11->GetMainDevice().GetContext()->OMSetRenderTargets(ARRAYSIZE(rtv), rtv, dx11->depth);

	dx11->GetMainDevice().GetContext()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	dx11->vsNone.SetGPU(dx11->GetMainDevice());
	dx11->gsDebugLine.SetGPU(dx11->GetMainDevice());
	dx11->psDebugLine.SetGPU(dx11->GetMainDevice());

	while (!lineDatas.empty())
	{
		const LineData& data = lineDatas.front();
		dx11->lineBuffer.SetGPU(data, dx11->GetMainDevice());
		dx11->GetMainDevice().GetContext()->Draw(1, 0);
		lineDatas.pop();
	}
}

void sf::debug::Debug::TraceLog()
{

	ImGui::Begin("Trace");

	ImVec4 color[]
	{
		ImVec4(1,1,1,1),
		ImVec4(0,1,0,1),
		ImVec4(1,1,0,1),
		ImVec4(1,0,0,1),
		ImVec4(1,0,1,1),
	};

	static bool showNormal = true;
	static bool showSafety = true;
	static bool showWarning = true;
	static bool showError = true;
	static bool showEngine = true;

	ImGui::Checkbox("Normal", &showNormal); ImGui::SameLine();
	ImGui::Checkbox("Safety", &showSafety); ImGui::SameLine();
	ImGui::Checkbox("Warning", &showWarning); ImGui::SameLine();
	ImGui::Checkbox("Error", &showError); ImGui::SameLine();
	ImGui::Checkbox("Engine", &showEngine);


	// 新しいログが追加されたときに自動スクロールするかどうか
	static bool autoScroll = true;
	if (ImGui::BeginPopup("Options")) {
		ImGui::Checkbox("Auto Scroll", &autoScroll);
		ImGui::EndPopup();
	}

	if (ImGui::Button("Options"))
		ImGui::OpenPopup("Options");
	ImGui::SameLine();
	bool clear = ImGui::Button("Clear");
	if (clear) logDatas.clear();

	ImGui::Separator();
	ImGui::BeginChild("LogList", ImVec2(0, 300), true, ImGuiWindowFlags_HorizontalScrollbar);

	// 現在のスクロール位置を取得
	float scrollY = ImGui::GetScrollY();
	float scrollMaxY = ImGui::GetScrollMaxY();

	//スレッド同期区間
	{
		std::lock_guard<std::mutex> lock(mtx);
		for (auto& i : logDatas) {
			// ログレベルによるフィルタリング
			if ((i.logKind == LogData::LogKind::Normal && !showNormal) ||
				(i.logKind == LogData::LogKind::Safety && !showSafety) ||
				(i.logKind == LogData::LogKind::Warning && !showWarning) ||
				(i.logKind == LogData::LogKind::Engine && !showEngine) ||
				(i.logKind == LogData::LogKind::Error && !showError)) {
				continue;
			}
			ImGui::TextColored(color[(int)i.logKind], (i.timeText + ":" + i.logu8).c_str());
		}
	}

	// 自動スクロール設定に基づいてスクロール
	if (autoScroll && scrollY >= scrollMaxY)
		ImGui::SetScrollHereY(1.0f);

	ImGui::EndChild();
	ImGui::End();
}

std::string sf::debug::Debug::GetTime()
{
	// 現在の時刻を取得（システムクロックのnow()メソッドを使用）
	auto now = std::chrono::system_clock::now();

	// 現在の時刻をtime_t形式に変換
	std::time_t now_time = std::chrono::system_clock::to_time_t(now);

	// time_tをローカル時間として文字列に変換
	std::tm local_tm = *std::localtime(&now_time);
	// ostringstreamを使ってフォーマットした文字列を生成
	std::ostringstream oss;
	oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");

	// 文字列に変換
	std::string time_str = oss.str();

	return time_str;
}

void sf::debug::Debug::AddLog(LogData data)
{
	//スレッド同期区間
	{
		std::lock_guard<std::mutex> lock(mtx);
		logDatas.push_back(data);


		textFile.Write({ data.timeText + " " + data.log });

		if (logDatas.size() > 100)logDatas.erase(logDatas.begin());

		std::cout << data.timeText << ":" << data.log << std::endl;
	}
}


std::string sf::ConvertToUTF8(const std::string& sjis_str) {
	// Shift-JIS -> UTF-16
	int utf16_size = MultiByteToWideChar(CP_ACP, 0, sjis_str.c_str(), -1, nullptr, 0);
	std::wstring utf16_str(utf16_size, 0);
	MultiByteToWideChar(CP_ACP, 0, sjis_str.c_str(), -1, &utf16_str[0], utf16_size);

	// UTF-16 -> UTF-8
	int utf8_size = WideCharToMultiByte(CP_UTF8, 0, utf16_str.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::string utf8_str(utf8_size, 0);
	WideCharToMultiByte(CP_UTF8, 0, utf16_str.c_str(), -1, &utf8_str[0], utf8_size, nullptr, nullptr);

	return utf8_str;
}

// UTF-8からUTF-16への変換
std::wstring sf::Utf8ToUtf16(const std::string& utf8)
{
	int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
	if (wlen == 0) return L"";

	std::wstring utf16(wlen, 0);
	MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &utf16[0], wlen);

	return utf16;
}

// UTF-16からShift-JISへの変換
std::string sf::Utf16ToShiftJIS(const std::wstring& utf16)
{
	int len = WideCharToMultiByte(CP_ACP, 0, utf16.c_str(), -1, nullptr, 0, nullptr, nullptr);
	if (len == 0) return "";

	std::string shiftJIS(len, 0);
	WideCharToMultiByte(CP_ACP, 0, utf16.c_str(), -1, &shiftJIS[0], len, nullptr, nullptr);

	return shiftJIS;
}

// UTF-8からShift-JISへの直接変換（ユーティリティ関数）
std::string sf::Utf8ToShiftJIS(const std::string& utf8)
{
	std::wstring utf16 = Utf8ToUtf16(utf8);  // UTF-8 -> UTF-16
	return Utf16ToShiftJIS(utf16);           // UTF-16 -> Shift-JIS
}
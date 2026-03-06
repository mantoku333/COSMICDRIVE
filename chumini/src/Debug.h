#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <queue>
#include "Vector3.h"
#include "File.h"

#define DEBUG_LINE	(true)

namespace sf
{

	//文字列をUTF8に変換する関数
	std::string ConvertToUTF8(const std::string& sjis_str);
	std::wstring Utf8ToUtf16(const std::string& utf8);
	std::string Utf16ToShiftJIS(const std::wstring& utf16);
	std::string Utf8ToShiftJIS(const std::string& utf8);

	namespace debug
	{
		//デバッグ処理
		class Debug
		{
		private:
			struct LogData
			{
				enum LogKind
				{
					Normal,
					Safety,
					Warning,
					Error,
					Engine,
				};

				//ログのテキスト
				std::string log;

				//ログのテキスト(u8コンバート後)
				std::string logu8;

				//時刻をテキスト化したもの
				std::string timeText;
				LogKind logKind = LogKind::Normal;
			};

		public:
			struct LineData
			{
				DirectX::XMFLOAT4 p1;
				DirectX::XMFLOAT4 p2;
				DirectX::XMFLOAT4 color;
			};

		public:
			/// <summary>
			/// ログを外部ファイルに書き込む
			/// </summary>
			static void WriteLogFile();

			/// <summary>
			/// ログの出力
			/// </summary>
			/// <param name="str"></param>
			static void Log(std::string str);
			static void Log(const Vector3& v);

			/// <summary>
			/// 成功ログの出力
			/// </summary>
			/// <param name="str"></param>
			static void LogSafety(std::string str);

			/// <summary>
			/// 警告ログの出力
			/// </summary>
			/// <param name="str"></param>
			static void LogWarning(std::string str);

			/// <summary>
			/// エラーログの出力
			/// </summary>
			/// <param name="str"></param>
			static void LogError(std::string str);

			/// <summary>
			/// エンジンログの出力
			/// </summary>
			/// <param name="str"></param>
			static void LogEngine(std::string str);

			/// <summary>
			/// 線の描画
			/// </summary>
			/// <param name="p1"></param>
			/// <param name="p2"></param>
			/// <param name="color"></param>
			static void DrawLine(Vector3 p1, Vector3 p2, DirectX::XMFLOAT4 color);

			static void DrawLog();

			/// <summary>
			/// 全ログの出力
			/// </summary>
			static void TraceLog();

		private:
			static std::string GetTime();

			static void AddLog(LogData data);

		private:
			static std::vector<LogData> logDatas;
			static std::queue<LineData> lineDatas;
			static std::mutex mtx;

			static sf::file::TextFile textFile;
		};
	}
}

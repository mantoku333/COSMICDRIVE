#pragma once
#define FRAME_RATE (60)
#include <Windows.h>
namespace sf
{
	//アプリケーションの時間管理(Staticクラス)
	class Time
	{
	public:
		static void Init();

		//時間の更新処理(1フレーム経過ならtrue)
		static bool Update();

		static long long GetPassTime() { return passTime; }

		//実際のFPS取得(設定値ではない)
		static double GetFPS();

		static float GetSetingFPS() { return setfps; }

		//FPSの設定
		static void SetFPS(float _fps);

		static float DeltaTime();

	private:

		static float setfps;

		// FPS表示用変数
		static int fpsCounter;
		static long long oldTick;
		static long long nowTick;

		// FPS固定用変数
		static LARGE_INTEGER liWork; // 関数から値を受け取る用
		static long long frequency; // 計測精度

		// １フレームあたりのカウント値を計算
		static long long numCount_1frame;

		// 現在時間（単位：カウント）を取得
		static long long oldCount;
		static long long nowCount;

		static long long passTime;
	};
}
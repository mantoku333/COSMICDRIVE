#include "Time.h"
float sf::Time::setfps = FRAME_RATE;

// FPS表示用変数
int sf::Time::fpsCounter;
long long sf::Time::oldTick;
long long sf::Time::nowTick;

// FPS固定用変数
LARGE_INTEGER sf::Time::liWork; // 関数から値を受け取る用
long long sf::Time::frequency; // 計測精度

// １フレームあたりのカウント値を計算
long long sf::Time::numCount_1frame;

// 現在時間（単位：カウント）を取得
long long sf::Time::oldCount;
long long sf::Time::nowCount;

long long sf::Time::passTime;

void sf::Time::Init()
{
	// FPS表示用変数
	fpsCounter = 0;
	oldTick = GetTickCount64();		//現在時間を保存
	nowTick = oldTick;				// 現在時間取得用

	// 計測精度を取得
	QueryPerformanceFrequency(&liWork);
	frequency = liWork.QuadPart;	// １秒あたりの解像度が入る

	// １フレームあたりのカウント値を計算
	numCount_1frame = frequency / FRAME_RATE;

	// 現在時間（単位：カウント）を取得
	QueryPerformanceCounter(&liWork);
	oldCount = liWork.QuadPart;
	nowCount = oldCount;
}

bool sf::Time::Update()
{
	// 1 / FRAME_RATE 秒経過したか
	QueryPerformanceCounter(&liWork);
	nowCount = liWork.QuadPart;			// 現在時間を取得（単位：カウント）

	if (nowCount >= oldCount + numCount_1frame)
	{
		fpsCounter++; // ゲームループ数を加算

		passTime = nowCount - oldCount;

		oldCount = nowCount;

		return true;
	}
	else
	{
		return false;
	}
}

double sf::Time::GetFPS()
{
	long long t = GetPassTime();
	t *= 0.001;
	double ret = t;
	ret = 1.0f / ret;
	ret *= 10000;
	return ret;
}

void sf::Time::SetFPS(float _fps)
{
	setfps = _fps;

	LARGE_INTEGER _liWork;
	// 計測精度を取得
	QueryPerformanceFrequency(&_liWork);
	long long _frequency = _liWork.QuadPart;	// １秒あたりの解像度が入る

	// １フレームあたりのカウント値を計算
	numCount_1frame = _frequency / _fps;
}

float sf::Time::DeltaTime()
{
	return static_cast<float>(1.0 / GetFPS());
}

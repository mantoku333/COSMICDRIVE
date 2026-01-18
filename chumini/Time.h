#pragma once
#define FRAME_RATE (120)
#include <Windows.h>
namespace sf
{
class Time
{
public:
static void Init();
static bool Update();
static long long GetPassTime() { return passTime; }
static double GetFPS();
static float GetSetingFPS() { return setfps; }
static void SetFPS(float _fps);
static float DeltaTime();

private:
static float setfps;
static int fpsCounter;
static long long oldTick;
static long long nowTick;
static LARGE_INTEGER liWork;
static long long frequency;
static long long numCount_1frame;
static long long oldCount;
static long long nowCount;
static long long passTime;
};
}
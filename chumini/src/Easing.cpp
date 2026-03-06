#include "Easing.h"
#include <cmath>

static double Linear(float x) { return x; }
static double EaseInSine(float x) { return 1 - cos((x * PI) / 2); }
static double EaseOutSine(float x) { return sin((x * PI) / 2); }
static double EaseInOutSine(float x) { return -(cos(PI * x) - 1) / 2; }
static double EaseInQuad(float x) { return x * x; }
static double EaseOutQuad(float x) { return 1 - (1 - x) * (1 - x); }
static double EaseInOutQuad(float x) { return x < 0.5f ? 2 * x * x : 1 - pow(-2 * x + 2, 2) / 2; }
static double EaseInCubic(float x) { return x * x * x; }
static double EaseOutCubic(float x) { return 1 - pow(1 - x, 3); }
static double EaseInOutCubic(float x) { return x < 0.5f ? 4 * x * x * x : 1 - pow(-2 * x + 2, 3) / 2; }
static double EaseInQuart(float x) { return x * x * x * x; }
static double EaseOutQuart(float x) { return 1 - pow(1 - x, 5); }
static double EaseInOutQuart(float x) { return x < 0.5f ? 8 * x * x * x * x : 1 - pow(-2 * x + 2, 4) / 2; }
static double EaseInQuint(float x) { return x * x * x * x * x; }
static double EaseOutQuint(float x) { return 1 - pow(1 - x, 4); }
static double EaseInOutQuint(float x) { return x < 0.5f ? 8 * x * x * x * x : 1 - pow(-2 * x + 2, 4) / 2; }
static double EaseInExpo(float x) { return x == 0 ? 0 : pow(2, 10 * x - 10); }
static double EaseOutExpo(float x) { return x == 1 ? 1 : 1 - pow(2, -10 * x); }
static double EaseInOutExpo(float x) { return x == 0 ? 0 : x == 1 ? 1 : x < 0.5 ? pow(2, 20 * x - 10) / 2 : (2 - pow(2, -20 * x + 10)) / 2; }
static double EaseInCirc(float x) { return 1 - sqrt(1 - pow(x, 2)); }
static double EaseOutCirc(float x) { return sqrt(1 - pow(x - 1, 2)); }
static double EaseInOutCirc(float x) { return x < 0.5f ? (1 - sqrt(1 - pow(2 * x, 2))) / 2 : (sqrt(1 - pow(-2 * x + 2, 2)) + 1) / 2; }
static double EaseInBack(float x) { return TTC3 * x * x * x - TTC1 * x * x; }
static double EaseOutBack(float x) { return 1 + TTC3 * pow(x - 1, 3) + TTC1 * pow(x - 1, 2); }
static double EaseInOutBack(float x) { return x < 0.5f ? (pow(2 * x, 2) * ((TTC2 + 1) * 2 * x - TTC2)) / 2 : (pow(2 * x - 2, 2) * ((TTC2 + 1) * (x * 2 - 2) + TTC2) + 2) / 2; }
static double EaseInElastic(float x) { return x == 0 ? 0 : x == 1 ? 1 : -pow(2, 10 * x - 10) * sin((x * 10 - 10.75) * TTC4); }
static double EaseOutElastic(float x) { return x == 0 ? 0 : x == 1 ? 1 : pow(2, -10 * x) * sin((x * 10 - 0.75) * TTC4) + 1; }
static double EaseInOutElastic(float x) { return x < 0.5 ? (pow(2 * x, 2) * ((TTC2 + 1) * 2 * x - TTC2)) / 2 : (pow(2 * x - 2, 2) * ((TTC2 + 1) * (x * 2 - 2) + TTC2) + 2) / 2; }
static double EaseOutBounce(float x) { return (x < 1 / TTD1) ? TTN1 * x * x : (x < 2 / TTD1) ? TTN1 * (x -= 1.5f / TTD1) * x + 0.75f : (x < 2.5f / TTD1) ? TTN1 * (x -= 2.25f / TTD1) * x + 0.9375f : TTN1 * (x -= 2.625f / TTD1) * x + 0.984375f; }
static double EaseInBounce(float x) { return 1 - EaseOutBounce(1 - x); }
static double EaseInOutBounce(float x) { return x < 0.5 ? (1 - EaseOutBounce(1 - 2 * x)) / 2 : (1 + EaseOutBounce(2 * x - 1)) / 2; }
static double EaseOutInSine(float x)
{
	return x < 0.5 ?
		-2 * pow(x - 0.5f, 2) + 0.5f :
		2 * pow(x - 0.5f, 2) + 0.5f;
}
static double Yoyo(float x) { return sin(PI * x); }

double (*EasingPtr[(int)EASE::Ease_end])(float _x) {
	Linear,
		EaseInSine,
		EaseOutSine,
		EaseInOutSine,
		EaseInQuad,
		EaseOutQuad,
		EaseInOutQuad,
		EaseInCubic,
		EaseOutCubic,
		EaseInOutCubic,
		EaseInQuart,
		EaseOutQuart,
		EaseInOutQuart,
		EaseInQuint,
		EaseOutQuint,
		EaseInOutQuint,
		EaseInExpo,
		EaseOutExpo,
		EaseInOutExpo,
		EaseInCirc,
		EaseOutCirc,
		EaseInOutCirc,
		EaseInBack,
		EaseOutBack,
		EaseInOutBack,
		EaseInElastic,
		EaseOutElastic,
		EaseInOutElastic,
		EaseOutBounce,
		EaseInBounce,
		EaseInOutBounce,
		EaseOutInSine,
		Yoyo,
};
double Easing(float _x, EASE ease) {
	return EasingPtr[int(ease)](_x);
}

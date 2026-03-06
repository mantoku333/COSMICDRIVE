#pragma once
//イージングチートシート https://easings.net/ja
#ifndef PI
#define PI	(3.141592653f)
#endif
#define TTC1	(1.70158f)
#define TTC2	(TTC1*1.525f)
#define TTC3	(TTC1+1)
#define TTC4	((2 * PI) / 3)
#define TTN1	(7.5625f)
#define TTD1	(2.75f)

	//イージング
enum class EASE {
	EaseLinear,
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

	EaseYoyo,

	Ease_end
};

double Easing(float _x, EASE ease);
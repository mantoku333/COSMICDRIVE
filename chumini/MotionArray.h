#pragma once
#include "MotionData.h"
#include <vector>
namespace sf
{
	namespace motion
	{
		//1つのキャラクターが使うモーションがまとまったクラス
		class MotionArray
		{
		public:
			virtual void LoadMotions() = 0;

			const std::vector<MotionData>& GetMotionDatas()const { return motionDatas; }

		protected:
			std::vector<MotionData> motionDatas;
		};
	}
}
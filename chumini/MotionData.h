#pragma once
#include <string>
#include <map>
#include <vector>
#include <DirectXMath.h>

namespace sf
{
	namespace motion
	{
		//1つのモーションデータ
		class MotionData
		{
		public:
			void LoadMotion(std::string path);

			DirectX::XMMATRIX GetMatrix(int frame, std::string nodeName)const;

			const std::vector<std::map<std::string, DirectX::XMMATRIX>>& GetData()const { return datas; }

			int Duration()const { return duration; }

		private:
			//モーションフレーム
			int duration = 0;

			std::vector<std::map<std::string, DirectX::XMMATRIX>> datas;
		};
	}
}
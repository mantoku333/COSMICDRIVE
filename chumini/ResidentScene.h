#pragma once
#include "App.h"
namespace app
{
	class ResidentScene :public sf::Scene<ResidentScene>
	{
	protected:
		void Init()override;

	private:
		/// <summary>
		/// ローディングシーンをロードする
		/// </summary>
		void LoadLoadingScene();
	};
}
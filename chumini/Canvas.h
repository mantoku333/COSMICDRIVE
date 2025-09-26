#pragma once
#include "Component.h"
#include "UI.h"
#include "Image.h"

namespace sf
{
	namespace ui
	{
		//2D描画用キャンバス
		class Canvas :public Component
		{
		public:
			~Canvas();
			void Begin()override;

			void Activate()override;
			void DeActivate()override;

			/// <summary>
			/// レイヤーの設定
			/// </summary>
			/// <param name="_layer"></param>
			void SetLayer(int _layer);
			const int& GetLayer()const { return layer; }

			static void DrawCanvasies();

		protected:
			/// <summary>
			/// UIの追加
			/// </summary>
			/// <typeparam name="T"></typeparam>
			/// <returns></returns>
			template<typename T>
			T* AddUI()
			{
				T* ret = new T();
				UI* ui = static_cast<UI*>(ret);
				uis.push_back(ui);
				return ret;
			}

		private:
			void Draw();

		private:
			static std::list<Canvas*> canvasies;
			static std::mutex mtx;

		private:
			int layer = 0;

			std::list<UI*> uis;
		};
	}
}

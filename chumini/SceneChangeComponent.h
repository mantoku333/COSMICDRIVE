#pragma once
#include "App.h"
namespace app
{
	namespace test
	{
		class SceneChangeComponent :public sf::Component
		{
		public:
			void Begin()override;
			//void ShowSelectScene();
			void ChangeScene();


            template<typename SceneType>
            void ChangeScene()
            {
                // 現在のシーンを完全に削除
                if (!scene.isNull())
                {
                    if (scene->IsActivate())
                    {
                        scene->DeActivate();
                    }
                    scene = nullptr;
                }

                // 新しいシーンを作成
                scene = SceneType::StandbyScene();

                // アクティブ化
                if (scene->StandbyThisScene())
                {
                    scene->Activate();
                }
                else
                {
                    sf::debug::Debug::LogWarning("シーン読み込みが完了していません！");
                }
            }


		private:
			void Update();

		private:
			//更新コマンド
			sf::command::Command<> updateCommand;

			//新規シーン
			sf::SafePtr<sf::IScene> scene;
		};
	}
}
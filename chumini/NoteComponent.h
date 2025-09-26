#pragma once
#include "Component.h"
#include "NoteData.h"

namespace app
{
    namespace test
    {
        class NoteComponent : public sf::Component {

        public:

            NoteComponent();

            void Begin() override;
			void Activate() override;
            void DeActivate() override;
            void Update(const sf::command::ICommand& command);

            // 外部からセットする譜面情報
            NoteData info;
            float    leadTime;         // dropTime から startY までのリード秒
            float    spawnTime;        // info.dropTime - leadTime
            float    elapsed = 0.f;          // 生成後の経過秒

            const float basenoteSpeed = 1.0f;              // 基本のノーツ速度
            float HiSpeed = 7.0f;                          // HiSpeed倍率

            float noteSpeed = basenoteSpeed * HiSpeed;     // 最終的なノーツ速度
        private:

            // 毎フレーム呼び出すコマンド
            sf::command::Command<> updateCommand;

            int lanes;
            float panelWidth;
            float startY;
            float hitY;

           
          

            // 初回読み込みを制限
            bool skipFirstActivate = true;

			bool isActive = false;

        };
    }
}

#include "ResultCanvas.h"
#include "JudgeStatsService.h"
#include "SelectScene.h" // 戻る先のシーン（適宜変更してください）
#include <string>

namespace app::test {

    // 定数（TestCanvasと合わせる）
    static constexpr float digitWidth = 100.0f;
    static constexpr float digitHeight = 100.0f;
    static constexpr float sheetWidth = 1000.0f;
    static constexpr float sheetHeight = 100.0f;

    void ResultCanvas::Begin() {
        sf::ui::Canvas::Begin();

        // ---------------------------------------------
        // リソース読み込み
        // ---------------------------------------------
        // 背景や数字の画像を読み込む
        textureBackground.LoadTextureFromFile("Assets/Texture/SelectBack.png"); // 適当な背景
        textureNumber.LoadTextureFromFile("Assets/Texture/numbers.png");

        // 背景表示
        auto bg = AddUI<sf::ui::Image>();
        bg->transform.SetPosition(Vector3(0, 0, 0));
        bg->transform.SetScale(Vector3(19.2f, 10.8f, 0)); // 画面全体
        bg->material.texture = &textureBackground;
        bg->material.SetColor({ 0.5f, 0.5f, 0.5f, 1.0f }); // 少し暗くする

        // ---------------------------------------------
        // JudgeStatsService から結果を取得
        // ---------------------------------------------
        int perfect = JudgeStatsService::GetCount(JudgeResult::Perfect);
        int great = JudgeStatsService::GetCount(JudgeResult::Great);
        int good = JudgeStatsService::GetCount(JudgeResult::Good);
        int miss = JudgeStatsService::GetCount(JudgeResult::Miss);
        int maxCombo = JudgeStatsService::GetMaxCombo(); // ※MaxComboの実装があれば

        // ---------------------------------------------
        // 数字の描画 (座標は画面に合わせて調整してください)
        // ---------------------------------------------

        // Perfect
        DrawNumber(perfect, 200, 300);

        // Great
        DrawNumber(great, 200, 150);

        // Good
        DrawNumber(good, 200, 0);

        // Miss
        DrawNumber(miss, 200, -150);

        // Max Combo (あれば)
        // DrawNumber(maxCombo, 600, 300);

        // 戻る準備
        if (nextScene.isNull()) {
            // 例: SelectSceneへ戻る
            nextScene = SelectScene::StandbyScene();
        }

        updateCommand.Bind(std::bind(&ResultCanvas::Update, this, std::placeholders::_1));
    }

    void ResultCanvas::Update(const sf::command::ICommand&) {

        // スペースキーなどでセレクト画面へ戻る
        if (SInput::Instance().GetKeyDown(Key::SPACE)) {
            if ( nextScene->StandbyThisScene()) {
                nextScene->Activate();

                auto owner = actorRef.Target();
                if (owner) owner->GetScene().DeActivate();
            }
        }
    }

    // 数字描画関数（指定座標にImageを生成して配置する）
    void ResultCanvas::DrawNumber(int number, float x, float y, float scale) {
        std::string str = std::to_string(number);
        float charSpacing = (digitWidth - 60.0f) * scale; // 文字間隔

        // 桁数分ループ
        for (size_t i = 0; i < str.size(); ++i) {
            int digit = str[i] - '0';

            // UV計算
            float u0 = (digit * digitWidth) / sheetWidth;
            float u1 = ((digit + 1) * digitWidth) / sheetWidth;
            float v0 = 0.0f;
            float v1 = digitHeight / sheetHeight;

            // UI生成
            auto img = AddUI<sf::ui::Image>();

            // 座標計算（左揃えの例）
            float posX = x + i * charSpacing;

            img->transform.SetPosition(Vector3(posX, y, 0));
            img->transform.SetScale(Vector3(scale, scale, 0));
            img->material.texture = &textureNumber;
            img->SetUV(u0, v0, u1, v1);
        }
    }
}
//#pragma once
//#include "Canvas.h"
//#include "NoteData.h"
//#include <vector>
//
//namespace sf { namespace ui { class Image; } }
//namespace app {
//    namespace test {
//        class EditScene;
//
//        class EditCanvas : public sf::ui::Canvas {
//        public:
//            void Begin() override;
//            void Update(const sf::command::ICommand&);
//
//        private:
//
//            sf::command::Command<> updateCommand;
//            // 所属シーン
//            EditScene* scene{ nullptr };
//
//            // --- ダミーUI ---
//            sf::ui::Image* backImage{};
//            sf::ui::Image* runButton{};
//            std::vector<sf::ui::Image*> dummyNoteButtons;
//
//            // テクスチャ（エンジン側型名が違う場合は置換）
//            sf::Texture textureBack;
//            sf::Texture textureDummyNote;
//            sf::Texture textureRun;
//
//            // 選択中ブラシ（パレットのインデックス）
//            int selectedBrushIdx{ 0 };
//
//            // ---- マウス座標（UI座標：原点=画面中心、+Y=上）----
//            Vector2 GetMousePositionUI();
//
//            // 画像の矩形ヒットテスト（TitleCanvasと同じ思想）
//            bool IsHovered(sf::ui::Image* img, float baseW, float baseH, const Vector2& mouseUI);
//
//            // ---- タイムライン（ダミー矩形） ----
//            // ここをクリックすると lane/MBT に変換してノーツを置く
//            // 位置は UI 座標。必要に応じて調整してね
//            static constexpr float TL_LEFT = -200.0f;
//            static constexpr float TL_RIGHT = 600.0f;
//            static constexpr float TL_BOTTOM = -250.0f;
//            static constexpr float TL_TOP = 250.0f;
//
//            // ヒットテストの基準サイズ（TitleCanvasに合わせた任意の値）
//            static constexpr float NOTE_BASE_W = 100.0f;
//            static constexpr float NOTE_BASE_H = 100.0f;
//            static constexpr float RUN_BASE_W = 200.0f;
//            static constexpr float RUN_BASE_H = 80.0f;
//
//            // ---- クリック→(lane,MBT) 変換ヘルパ ----
//            bool UiToLaneMBT(const Vector2& ui, int& outLane, Mbt16& outMBT) const;
//
//            // グリッド設定（拡大縮小やスクロールしたくなったらここを操作）
//            int   lanes{ 6 };
//            float pixelsPerBeat{ 64.0f }; // 1拍の幅(px)
//            int   scrollMeasure{ 0 };     // 表示先頭の小節
//
//        private:
//            // 入力系：キーボード置き/削除・保存/読込・再生トグルなどは cpp 側に実装
//        };
//
//    }
//} // namespace

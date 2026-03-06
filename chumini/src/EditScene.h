#pragma once
#include "App.h"
#include"NoteData.h"

namespace app
{
    namespace test
    {
        class EditScene :public sf::Scene<EditScene>
        {
        public:
            sf::ref::Ref<sf::Actor> uiManagerActor;
            void Init() override;
            void Update(const sf::command::ICommand& command);

        private:
            sf::command::Command<> updateCommand;


        public:
            static constexpr const char* kChartPath = "Assets/Save/chart/goodtek.chart";
            static constexpr const char* kMusicPath = "Assets/Save/music/GOODTEK.wav";

            float GetSongTimeSec() const { return songTimeSec; }
            bool  IsPlaying() const { return isPlaying; }

            std::vector<NoteData>& GetChart() { return chart; }
            const std::vector<NoteData>& GetChart() const { return chart; }

            const float panelW = 20.0f;
            const float panelH = 30.0f;

            const Vector3 panelPos{ -5.0f, 0.0f, 10.0f };  // ← ここだけ使う

            int   lanes = 5;
            const float laneW = panelW / lanes;

            const float noteW = laneW * 0.9f;
            const float noteH = 0.2f;
            const float noteD = 0.1f;

            float barY;
            const float barH = 0.1f;

            std::vector<sf::ref::Ref<sf::Actor>> lanePanels;


            sf::geometry::GeometryCube g_cube;

        private:
            std::unique_ptr<sf::sound::SoundPlayer> bgm;
            sf::sound::SoundResource bgmRes;

            bool  isPlaying{ false };
            float songTimeSec{ 0.0f };
            float songLengthSec{ 0.0f };

            std::vector<NoteData> chart;

            // カーソル（配置予定の位置）
            struct CursorState {
                int   lane = 0;
                Mbt16 mbt = { 0,0,0 };
            };
            CursorState cursor_;

            // 選択中のノーツインデックス（未選択: -1）
            int selectedIndex_ = -1;

            // 縦スクロールと表示小節数（必要に応じて使う）
            float scrollMeasure_ = 0.0f;
            int   visibleMeasures_ = 8;

            // ── クリック操作用の内部関数 ──
            void SelectOrPlaceByClick();
            void PlaceNoteAtCursor();
            void DeleteSelected();
            int  HitTestNote(int lane, const Mbt16& mbt) const;

            // ── 変換系ヘルパ ──
            bool ScreenToPanel(float mx, float my, Vector3& outWorld) const;
            bool WorldToLaneMbt(const Vector3& wpos, int& outLane, Mbt16& outMbt) const;
            bool GetMouseClientPos(int& outX, int& outY) const;
        };
    }
}
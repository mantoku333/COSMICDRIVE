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

            void TransportPlay();
            void TransportStop();
            void TransportSeek(float sec);

            std::vector<NoteData>& GetChart() { return chart; }
            const std::vector<NoteData>& GetChart() const { return chart; }

            bool SaveChartToFile();
            bool LoadChartFromFile();

        private:
            std::unique_ptr<sf::sound::SoundPlayer> bgm;
            sf::sound::SoundResource bgmRes;

            bool  isPlaying{ false };
            float songTimeSec{ 0.0f };
            float songLengthSec{ 0.0f };

            std::vector<NoteData> chart;
        };
    }
}
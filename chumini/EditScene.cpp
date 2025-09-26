#include "EditScene.h"
#include "EditCanvas.h"
#include "SInput.h"
#include "Debug.h"
#include "Time.h"          // DeltaTime を使う想定
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdio>

using namespace app::test;

void EditScene::Init()
{
    // ---- UI キャンバス（エディタ本体） ----
    uiManagerActor = Instantiate();
    auto editCanvas = uiManagerActor.Target()->AddComponent<EditCanvas>();
    uiManagerActor.Target()->transform.SetPosition({ 0.0f, 0.0f, 0.0f });

    // ---- Update 登録 ----
    updateCommand.Bind(std::bind(&EditScene::Update, this, std::placeholders::_1));

    // ---- 既存譜面のロード（固定パス）----
    if (!LoadChartFromFile()) {
        sf::debug::Debug::Log("EditScene: 初回ロード失敗（新規開始）");
    }

    sf::debug::Debug::Log("EditScene 初期化完了");
}

void EditScene::Update(const sf::command::ICommand& /*command*/)
{
    const float dt = sf::Time::DeltaTime();

    if (isPlaying) {
        songTimeSec += dt;

        // 長さが設定されていれば自動停止
        if (songLengthSec > 0.0f && songTimeSec >= songLengthSec) {
            TransportStop();
        }
    }
}

void EditScene::TransportPlay()
{
    if (isPlaying) return;
    isPlaying = true;
    // サウンド無しなので songTimeSec の進行だけ行う
}

void EditScene::TransportStop()
{
    isPlaying = false;
    songTimeSec = 0.0f;
}

void EditScene::TransportSeek(float sec)
{
    if (sec < 0.0f) sec = 0.0f;
    if (songLengthSec > 0.0f) {
        if (sec > songLengthSec) sec = songLengthSec;
    }
    songTimeSec = sec;
}

// -------------------------------------------------
// 固定パスへ保存 (CSV: lane,measure,beat,tick16,type)
// -------------------------------------------------
bool EditScene::SaveChartToFile()
{
    std::ofstream ofs(kChartPath);
    if (!ofs) {
        sf::debug::Debug::Log("EditScene: Save 失敗（ファイル開けず）");
        return false;
    }

    // 一応ソート（measure→beat→tick→lane）
    std::sort(chart.begin(), chart.end(), [](const NoteData& a, const NoteData& b) {
        if (a.mbt.measure != b.mbt.measure) return a.mbt.measure < b.mbt.measure;
        if (a.mbt.beat != b.mbt.beat) return a.mbt.beat < b.mbt.beat;
        if (a.mbt.tick16 != b.mbt.tick16) return a.mbt.tick16 < b.mbt.tick16;
        return a.lane < b.lane;
        });

    for (const auto& ev : chart) {
        ofs << ev.lane << ","
            << ev.mbt.measure << ","
            << ev.mbt.beat << ","
            << ev.mbt.tick16 << ","
            << static_cast<int>(ev.type) << "\n";
    }

    sf::debug::Debug::Log("EditScene: Save 成功");
    return true;
}

// -------------------------------------------------
// 固定パスから読込 (CSV: lane,measure,beat,tick16,type)
// -------------------------------------------------
bool EditScene::LoadChartFromFile()
{
    std::ifstream ifs(kChartPath);
    if (!ifs) {
        sf::debug::Debug::Log("EditScene: Load 失敗（ファイル無し）");
        return false;
    }

    std::vector<NoteData> temp;
    std::string line;

    while (std::getline(ifs, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        NoteData ev{};
        char comma;
        int typeI = 0;

        if (ss >> ev.lane >> comma
            >> ev.mbt.measure >> comma
            >> ev.mbt.beat >> comma
            >> ev.mbt.tick16 >> comma
            >> typeI) {
            ev.type = static_cast<NoteType>(typeI);
            temp.push_back(ev);
        }
    }

    chart.swap(temp);
    return true;
}

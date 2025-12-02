#include "ChedParser.h"
#include "Debug.h"
#include <fstream>
#include <sstream>
#include <cctype>
#include <vector>
#include <algorithm>
#include <utility>

namespace app::test {

    // -----------------------------
    // base36 1文字 -> int 変換
    // -----------------------------
    static int DecodeBase36Char(char c)
    {
        if ('0' <= c && c <= '9') return c - '0';
        if ('A' <= c && c <= 'Z') return c - 'A' + 10;
        if ('a' <= c && c <= 'z') return c - 'a' + 10;
        return 0;
    }

    struct BarInfo {
        int measureStart;     // この設定が有効になる最初の小節番号
        int ticksPerMeasure;  // その小節の1小節あたりtick数
        int startTick;        // この小節番号のときの先頭tick
    };

    bool ChedParser::Load(const std::string& path)
    {
        sf::debug::Debug::Log("ChedParser::Load: " + path);

        std::ifstream file(path);
        if (!file) {
            sf::debug::Debug::LogError("Ched譜面読み込み失敗: " + path);
            return false;
        }

        // 既存データをクリア
        notes.clear();
        tempos.clear();
        bpmTable.clear();

        // デフォルト tick
        ticksPerBeat = 480;

        std::string line;

        auto trim = [](std::string& s) {
            auto first = s.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) {
                s.clear();
                return;
            }
            auto last = s.find_last_not_of(" \t\r\n");
            s = s.substr(first, last - first + 1);
            };

        // ヘッダ + データ の一覧
        std::vector<std::pair<std::string, std::string>> scoreLines;

        // -----------------------------
        // 1パス目: 行を読み込んで分類
        // -----------------------------
        while (std::getline(file, line)) {

            trim(line);
            if (line.empty() || line.rfind("//", 0) == 0)
                continue;

            //----------------------------------------------------------
            // #REQUEST "ticks_per_beat 480"
            //----------------------------------------------------------
            if (line.rfind("#REQUEST", 0) == 0) {

                size_t q1 = line.find('"');
                size_t q2 = line.find('"', q1 + 1);

                if (q1 != std::string::npos && q2 != std::string::npos) {
                    std::string content = line.substr(q1 + 1, q2 - q1 - 1);

                    if (content.rfind("ticks_per_beat", 0) == 0) {
                        std::istringstream ss(content);
                        std::string key;
                        int value = ticksPerBeat;
                        ss >> key >> value;

                        ticksPerBeat = value;
                        sf::debug::Debug::Log("ticks_per_beat 読み取り: " + std::to_string(value));
                    }
                }
                continue;
            }

            // ここから下は「#...: ...」形式の譜面系行だけ拾う
            if (line[0] != '#')
                continue;

            auto colonPos = line.find(':');
            if (colonPos == std::string::npos) {
                // メタデータ系 (#TITLE 等) は今は無視
                continue;
            }

            std::string head = line.substr(1, colonPos - 1); // "#" を抜いたヘッダ
            std::string data = line.substr(colonPos + 1);
            trim(data);

            if (data.empty())
                continue;

            // SUS の例では "1400 0000" のように空白が入ることがあるので、
            // ここで一度全部の空白を取り除いておく
            data.erase(
                std::remove_if(data.begin(), data.end(),
                    [](unsigned char c) { return std::isspace(c) != 0; }),
                data.end()
            );

            if (data.empty())
                continue;

            scoreLines.emplace_back(std::move(head), std::move(data));
        }

        // -----------------------------
        // 2パス目: 小節長(#mmm02)テーブルを構築
        // -----------------------------
        std::vector<std::pair<int, double>> barLengths; // (measure, beats)

        for (auto& h : scoreLines) {
            const std::string& head = h.first;
            const std::string& data = h.second;

            if (head.size() == 5 &&
                std::isdigit(static_cast<unsigned char>(head[0])) &&
                std::isdigit(static_cast<unsigned char>(head[1])) &&
                std::isdigit(static_cast<unsigned char>(head[2])) &&
                head.substr(3, 2) == "02")
            {
                int measure = std::stoi(head.substr(0, 3));
                double beats = std::stod(data);
                barLengths.emplace_back(measure, beats);
            }
        }

        if (barLengths.empty()) {
            // 1個も指定が無ければ #00002:4 と同等扱い
            sf::debug::Debug::Log("No #mmm02 found. Using default 4/4 from measure 0.");
            barLengths.emplace_back(0, 4.0);
        }

        std::sort(barLengths.begin(), barLengths.end(),
            [](auto& a, auto& b) { return a.first < b.first; });

        std::vector<BarInfo> bars;
        int accumTicks = 0;

        for (size_t i = 0; i < barLengths.size(); ++i) {
            int measure = barLengths[i].first;
            double beats = barLengths[i].second;

            if (i == 0) {
                accumTicks = 0;
            }
            else {
                int prevMeasure = barLengths[i - 1].first;
                double prevBeats = barLengths[i - 1].second;
                accumTicks += static_cast<int>((measure - prevMeasure) * prevBeats * ticksPerBeat);
            }
            int tpm = static_cast<int>(beats * ticksPerBeat);

            bars.push_back(BarInfo{ measure, tpm, accumTicks });
        }

        // 指定小節に対応する BarInfo を引くヘルパー
        auto findBar = [&](int measure) -> const BarInfo& {
            const BarInfo* current = &bars.front();
            for (auto& b : bars) {
                if (b.measureStart <= measure) current = &b;
                else break;
            }
            return *current;
            };

        // SUSの「データ部 index→tick」に相当するやつ
        auto ToGlobalTick = [&](int measure, int index, int total) -> int {
            const BarInfo& bar = findBar(measure);
            int tpm = bar.ticksPerMeasure;
            int baseTick = bar.startTick +
                (measure - bar.measureStart) * tpm;
            int localTick = (index * tpm) / total;
            return baseTick + localTick;
            };

        // -----------------------------
        // BPM 定義 (#BPMzz: value)
        // -----------------------------
        for (auto& h : scoreLines) {
            const std::string& head = h.first;
            const std::string& data = h.second;

            if (head.size() == 5 && head.rfind("BPM", 0) == 0) {
                std::string idStr = head.substr(3, 2); // zz
                // base36 で ID を int にする
                int id = 0;
                for (char c : idStr) {
                    id = id * 36 + DecodeBase36Char(c);
                }
                double bpm = std::stod(data);
                bpmTable[id] = bpm;

                sf::debug::Debug::Log("BPM定義: ID=" + std::to_string(id) +
                    " BPM=" + std::to_string(bpm));
            }
        }

        // -----------------------------
        // 生の BPM 変更イベント／ノートを走査
        // -----------------------------
        for (auto& h : scoreLines) {
            const std::string& head = h.first;
            const std::string& data = h.second;

            // 小節ヘッダじゃないものはスキップ
            if (head.size() < 5 ||
                !std::isdigit(static_cast<unsigned char>(head[0])) ||
                !std::isdigit(static_cast<unsigned char>(head[1])) ||
                !std::isdigit(static_cast<unsigned char>(head[2])))
                continue;

            int mm = std::stoi(head.substr(0, 3));
            char kind = head[3];

            // データを2文字ずつに分割
            const int total = static_cast<int>(data.size() / 2);
            if (total <= 0) continue;

            // ----------------------------------------
            // BPM 変化 (#mmm08: zz...)
            // ----------------------------------------
            if (head.substr(3, 2) == "08") {
                for (int i = 0; i < total; ++i) {
                    std::string token = data.substr(i * 2, 2);
                    if (token == "00") continue;

                    // token は BPMID (base36)
                    int bpmId = 0;
                    for (char c : token) {
                        bpmId = bpmId * 36 + DecodeBase36Char(c);
                    }

                    auto it = bpmTable.find(bpmId);
                    if (it == bpmTable.end())
                        continue;

                    int tick = ToGlobalTick(mm, i, total);
                    double absBeat = tick / static_cast<double>(ticksPerBeat);

                    ChedTempoEvent e;
                    e.absBeat = absBeat;
                    e.bpm = it->second;
                    tempos.push_back(e);

                    sf::debug::Debug::Log("BPM変更: beat=" +
                        std::to_string(absBeat) + " BPM=" + std::to_string(e.bpm));
                }
                continue;
            }

            // ----------------------------------------
            // タップノート (#mmm1x: DATA)
            // （他の種類は今は無視 or 後で拡張）
            // ----------------------------------------
            if (kind == '1' && head.size() >= 5) {

                char laneChar = head[4];
                int lane = DecodeBase36Char(laneChar);

                const BarInfo& bar = findBar(mm);
                int tpm = bar.ticksPerMeasure;
                int barStartTick = bar.startTick +
                    (mm - bar.measureStart) * tpm;

                for (int i = 0; i < total; ++i) {
                    std::string token = data.substr(i * 2, 2);
                    if (token == "00") continue;

                    char typeChar = token[0];
                    char widthChar = token[1];

                    int noteType = DecodeBase36Char(typeChar);
                    int width = DecodeBase36Char(widthChar);

                    if (width <= 0)
                        continue; // 幅0はノート無し

                    int localTick = (i * tpm) / total;
                    int globalTick = barStartTick + localTick;

                    double beatInMeasure = localTick / static_cast<double>(ticksPerBeat);
                    double absBeat = globalTick / static_cast<double>(ticksPerBeat);

                    ChedNote n;
                    n.lane = lane;
                    n.measure = mm;
                    n.tick = localTick;   // 小節頭からのtick
                    n.beat = beatInMeasure;
                    n.absBeat = absBeat;

                    notes.push_back(n);

                    sf::debug::Debug::Log("ノート lane=" + std::to_string(n.lane) +
                        " absBeat=" + std::to_string(n.absBeat));
                }

                continue;
            }

            // ----------------------------------------
            // TODO: kind == '2' '3' '5' などは、
            // sus-io のロジックを参考にホールド/スライド/ディレクショナルに
            // 拡張できる余地あり
            // ----------------------------------------
        }

        // -----------------------------
        // ここで notes / tempos を実データごとソート
        // -----------------------------
        std::sort(notes.begin(), notes.end(),
            [](const ChedNote& a, const ChedNote& b) {
                if (a.absBeat != b.absBeat)
                    return a.absBeat < b.absBeat;
                if (a.lane != b.lane)
                    return a.lane < b.lane;
                if (a.measure != b.measure)
                    return a.measure < b.measure;
                return a.tick < b.tick;
            });

        std::sort(tempos.begin(), tempos.end(),
            [](const ChedTempoEvent& a, const ChedTempoEvent& b) {
                return a.absBeat < b.absBeat;
            });

        {
            sf::debug::Debug::Log("===== notes (time-sorted) =====");
            for (const auto& n : notes) {
                sf::debug::Debug::Log(
                    "beat=" + std::to_string(n.absBeat) +
                    " lane=" + std::to_string(n.lane) +
                    " measure=" + std::to_string(n.measure)
                );
            }
        }

        sf::debug::Debug::Log("Ched読み込み成功: ノーツ数 = " + std::to_string(notes.size()) +
            " / テンポイベント数 = " + std::to_string(tempos.size()));
        return true;
    }

} // namespace app::test

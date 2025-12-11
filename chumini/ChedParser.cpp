#include "ChedParser.h"
#include "Debug.h"
#include <Windows.h> 
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <filesystem>


namespace app::test {

    // =========================================================
    // 文字コード変換ヘルパ (UTF-8/SJIS -> Unicode -> UTF-8)
    // =========================================================

    // wstring (UTF-16) -> string (UTF-8)
    static std::string WStringToUtf8(const std::wstring& wstr) {
        if (wstr.empty()) return "";
        int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string str(size, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], size, nullptr, nullptr);
        if (!str.empty() && str.back() == '\0') str.pop_back();
        return str;
    }

    // string (Unknown) -> wstring (UTF-16)
    static std::wstring AnyToWString(const std::string& str) {
        if (str.empty()) return L"";

        int sizeUtf8 = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.c_str(), -1, nullptr, 0);
        if (sizeUtf8 > 0) {
            std::wstring wstr(sizeUtf8, 0);
            MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], sizeUtf8);
            if (!wstr.empty() && wstr.back() == L'\0') wstr.pop_back();
            return wstr;
        }

        int sizeAcp = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
        if (sizeAcp > 0) {
            std::wstring wstr(sizeAcp, 0);
            MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wstr[0], sizeAcp);
            if (!wstr.empty() && wstr.back() == L'\0') wstr.pop_back();
            return wstr;
        }

        return L"";
    }

    // ログ出力用
    static std::string WStringToLog(const std::wstring& wstr) {
        if (wstr.empty()) return "";
        int size = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (size <= 0) return "<???>";
        std::string str(size, 0);
        WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &str[0], size, nullptr, nullptr);
        if (!str.empty() && str.back() == '\0') str.pop_back();
        return str;
    }

    static int DecodeBase36Char(char c) {
        if ('0' <= c && c <= '9') return c - '0';
        if ('A' <= c && c <= 'Z') return c - 'A' + 10;
        if ('a' <= c && c <= 'z') return c - 'a' + 10;
        return 0;
    }

    static void TrimW(std::wstring& s) {
        auto first = s.find_first_not_of(L" \t\r\n");
        if (first == std::wstring::npos) { s.clear(); return; }
        auto last = s.find_last_not_of(L" \t\r\n");
        s = s.substr(first, last - first + 1);
    }

    static std::string RemoveQuotesAndToUtf8(std::wstring s) {
        if (s.size() >= 2 && s.front() == L'"' && s.back() == L'"') {
            s = s.substr(1, s.size() - 2);
        }
        return WStringToUtf8(s);
    }

    struct BarInfo { int measureStart; int ticksPerMeasure; int startTick; };

    // ========================================================================
    // Load
    // ========================================================================
    bool ChedParser::Load(const std::wstring& path, bool headerOnly)
    {
        // 初期化
        title = ""; artist = ""; jacketFile = ""; waveFile = "";
        level = 0; difficulty = 0; bpm = 0; // ★初期化
        notes.clear(); tempos.clear(); bpmTable.clear();
        ticksPerBeat = 480;

        sf::debug::Debug::Log("ChedParser::Load: " + WStringToLog(path) + (headerOnly ? " (HeaderOnly)" : ""));

        std::ifstream file(path, std::ios::binary);
        if (!file) {
            sf::debug::Debug::LogError("Ched譜面読み込み失敗");
            return false;
        }

        std::string rawLine;
        std::vector<std::pair<std::string, std::string>> scoreLines;

        // -----------------------------
        // 1パス目: 行読み込み & メタデータ解析
        // -----------------------------
        while (std::getline(file, rawLine)) {
            if (!rawLine.empty() && rawLine.back() == '\r') rawLine.pop_back();
            if (rawLine.empty()) continue;

            std::wstring line = AnyToWString(rawLine);
            TrimW(line);

            if (line.empty() || line.find(L"//") == 0) continue;

            // メタデータ解析 (#コマンド)
            if (line[0] == L'#') {
                std::wstring cmd, val;
                size_t sp = line.find_first_of(L" \t");
                size_t cl = line.find(L':');
                if (cl != std::wstring::npos && (sp == std::wstring::npos || cl < sp)) sp = cl;

                if (sp != std::wstring::npos) {
                    cmd = line.substr(0, sp);
                    val = line.substr(sp + 1);
                    TrimW(val);
                }
                else {
                    cmd = line;
                }

                if (cmd == L"#TITLE") { title = RemoveQuotesAndToUtf8(val); continue; }
                if (cmd == L"#ARTIST") { artist = RemoveQuotesAndToUtf8(val); continue; }
                if (cmd == L"#JACKET") { jacketFile = RemoveQuotesAndToUtf8(val); continue; }
                if (cmd == L"#WAVE") { waveFile = RemoveQuotesAndToUtf8(val); continue; }
                if (cmd == L"#PLAYLEVEL") { try { level = std::stoi(val); } catch (...) {} continue; }
                if (cmd == L"#DIFFICULTY") { try { difficulty = std::stoi(val); } catch (...) {} continue; }

                if (cmd == L"#REQUEST") {
                    if (val.find(L"ticks_per_beat") != std::wstring::npos) {
                        size_t nPos = val.find_last_of(L" \t");
                        if (nPos != std::wstring::npos) {
                            try { ticksPerBeat = std::stoi(val.substr(nPos + 1)); }
                            catch (...) {}
                        }
                    }
                    continue;
                }

                // ★追加: BPMを見つけたら即確保して、headerOnlyなら終了！
                // "#BPM" で始まるコマンド (例: #BPM01)
                if (cmd.find(L"#BPM") == 0) {
                    try {
                        double d = std::stod(val);
                        // 最初に見つかったBPMを採用（すでにセットされていれば上書きしないなど調整可）
                        if (this->bpm == 0) {
                            this->bpm = static_cast<int>(d);
                        }
                    }
                    catch (...) {}

                    // ヘッダ読み込みモードなら、BPMが取れた時点で満足して帰る
                    if (headerOnly && this->bpm > 0) {
                        return true;
                    }
                }
            }

            // ★削除: ここにあった「headerOnlyなら #00xxx で抜ける」処理は削除しました
            // これでファイル後半にあるBPM定義まで読み進めることができます。

            // 譜面データをバッファリング
            if (line[0] == L'#') {
                auto colonPos = line.find(L':');
                if (colonPos != std::wstring::npos) {
                    std::wstring headW = line.substr(1, colonPos - 1);
                    std::wstring dataW = line.substr(colonPos + 1);
                    TrimW(dataW);
                    dataW.erase(std::remove_if(dataW.begin(), dataW.end(), [](wchar_t c) { return iswspace(c); }), dataW.end());

                    if (!dataW.empty()) {
                        std::string head(headW.begin(), headW.end());
                        std::string data(dataW.begin(), dataW.end());
                        scoreLines.emplace_back(std::move(head), std::move(data));
                    }
                }
            }
        }

        // ここまで来たらファイル末尾 (headerOnly でもBPMが見つからなかった場合など)
        if (headerOnly) return true;

        // =================================================================
        // 譜面解析 (Full Load)
        // =================================================================

        std::vector<std::pair<int, double>> barLengths;
        for (auto& h : scoreLines) {
            if (h.first.size() == 5 && h.first.substr(3, 2) == "02") {
                try {
                    int measure = std::stoi(h.first.substr(0, 3));
                    double beats = std::stod(h.second);
                    barLengths.emplace_back(measure, beats);
                }
                catch (...) {}
            }
        }
        if (barLengths.empty()) barLengths.emplace_back(0, 4.0);

        std::sort(barLengths.begin(), barLengths.end(), [](auto& a, auto& b) { return a.first < b.first; });

        std::vector<BarInfo> bars;
        int accumTicks = 0;
        for (size_t i = 0; i < barLengths.size(); ++i) {
            int measure = barLengths[i].first;
            double beats = barLengths[i].second;
            if (i > 0) {
                int prevMeasure = barLengths[i - 1].first;
                double prevBeats = barLengths[i - 1].second;
                accumTicks += static_cast<int>((measure - prevMeasure) * prevBeats * ticksPerBeat);
            }
            bars.push_back(BarInfo{ measure, static_cast<int>(beats * ticksPerBeat), accumTicks });
        }

        auto findBar = [&](int measure) -> const BarInfo& {
            const BarInfo* current = &bars.front();
            for (auto& b : bars) {
                if (b.measureStart <= measure) current = &b; else break;
            }
            return *current;
            };

        auto ToGlobalTick = [&](int measure, int index, int total) -> int {
            const BarInfo& bar = findBar(measure);
            int tpm = bar.ticksPerMeasure;
            int baseTick = bar.startTick + (measure - bar.measureStart) * tpm;
            return baseTick + (index * tpm) / total;
            };

        // BPMテーブル構築
        for (auto& h : scoreLines) {
            if (h.first.size() == 5 && h.first.rfind("BPM", 0) == 0) {
                std::string idStr = h.first.substr(3, 2);
                int id = 0;
                for (char c : idStr) id = id * 36 + DecodeBase36Char(c);
                try { bpmTable[id] = std::stod(h.second); }
                catch (...) {}
            }
        }

        // Notes
        for (auto& h : scoreLines) {
            if (h.first.size() < 5 || !std::isdigit(h.first[0])) continue;

            int mm = 0;
            try { mm = std::stoi(h.first.substr(0, 3)); }
            catch (...) { continue; }
            std::string ch = h.first.substr(3, 2);
            const std::string& data = h.second;
            const int total = (int)data.size() / 2;
            if (total <= 0) continue;

            if (ch == "08") {
                for (int i = 0; i < total; ++i) {
                    std::string token = data.substr(i * 2, 2);
                    if (token == "00") continue;
                    int bpmId = 0;
                    for (char c : token) bpmId = bpmId * 36 + DecodeBase36Char(c);

                    if (bpmTable.count(bpmId)) {
                        int tick = ToGlobalTick(mm, i, total);
                        ChedTempoEvent e;
                        e.absBeat = tick / (double)ticksPerBeat;
                        e.bpm = bpmTable[bpmId];
                        tempos.push_back(e);
                    }
                }
                continue;
            }

            if (ch[0] == '1') {
                char laneChar = ch[1];
                int rawLane = DecodeBase36Char(laneChar);
                int standardLane = std::clamp(rawLane / 4, 0, 3);

                for (int i = 0; i < total; ++i) {
                    std::string token = data.substr(i * 2, 2);
                    if (token == "00") continue;

                    NoteType type = NoteType::Tap;
                    int targetLane = standardLane;

                    if (token == "44") type = NoteType::SongEnd;
                    else if (token == "24") targetLane = 4;
                    else {
                        if (DecodeBase36Char(token[1]) <= 0) continue;
                    }

                    int tick = ToGlobalTick(mm, i, total);
                    ChedNote n;
                    n.type = type;
                    n.lane = targetLane;
                    n.measure = mm;
                    n.tick = (i * findBar(mm).ticksPerMeasure) / total;
                    n.absBeat = tick / (double)ticksPerBeat;
                    notes.push_back(n);
                }
            }
        }

        std::sort(notes.begin(), notes.end(), [](const ChedNote& a, const ChedNote& b) {
            return a.absBeat < b.absBeat;
            });
        std::sort(tempos.begin(), tempos.end(), [](const ChedTempoEvent& a, const ChedTempoEvent& b) {
            return a.absBeat < b.absBeat;
            });

        // Time Calc & BPM Finalize
        {
            double currentBpm = 120.0;
            if (!bpmTable.empty()) {
                if (bpmTable.count(1)) currentBpm = bpmTable[1];
                else currentBpm = bpmTable.begin()->second;
            }

            // もし headerOnly でBPMが取れていなくても、ここで確実にセットする
            if (this->bpm == 0) {
                this->bpm = static_cast<int>(currentBpm);
            }

            sf::debug::Debug::Log("Initial BPM: " + std::to_string(currentBpm));

            double currentTime = 0.0;
            double lastEventBeat = 0.0;
            auto tempoIt = tempos.begin();

            if (tempoIt != tempos.end() && tempoIt->absBeat == 0.0) {
                currentBpm = tempoIt->bpm;
                tempoIt++;
            }

            for (auto& note : notes) {
                while (tempoIt != tempos.end() && tempoIt->absBeat <= note.absBeat) {
                    double beatDist = tempoIt->absBeat - lastEventBeat;
                    currentTime += beatDist * (60.0 / currentBpm);
                    lastEventBeat = tempoIt->absBeat;
                    currentBpm = tempoIt->bpm;
                    tempoIt++;
                }
                double dist = note.absBeat - lastEventBeat;
                note.time = currentTime + dist * (60.0 / currentBpm);
            }
        }

        sf::debug::Debug::Log("ChedFullLoad完了: Notes=" + std::to_string(notes.size()));
        return true;
    }

} // namespace app::test
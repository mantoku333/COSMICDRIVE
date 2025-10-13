//#include "EditCanvas.h"
//#include "EditScene.h"
//#include "NoteData.h"
//
//#include "SInput.h"
//#include "Time.h"
//#include "Debug.h"
//
//#include <algorithm>
//#include <Windows.h> // GetCursorPos / ScreenToClient / GetActiveWindow
//
//namespace app::test {
//
//    // ── カーソル（配置対象の lane/MBT を内部保持）
//    struct EditCursor {
//        int   lane = 0;         // 0..(lanes-1)
//        Mbt16 mbt = { 0,0,0 };   // 4/4・16分
//    };
//
//    // このcpp内だけで持つ状態
//    static EditCursor s_cursor;
//    static NoteType   s_brush = NoteType::Tap;
//    static int        s_lanes = 6;
//
//    // ---- 便利ヘルパ ----
//    static inline bool CtrlHeld() {
//        auto& I = SInput::Instance();
//        return I.GetKey(Key::KEY_CTRL);
//    }
//    static inline void ClampCursor(EditCursor& c) {
//        if (c.mbt.measure < 0) c.mbt.measure = 0;
//        c.mbt.beat = std::clamp(c.mbt.beat, 0, 3);
//        c.mbt.tick16 = std::clamp(c.mbt.tick16, 0, 3);
//        c.lane = std::clamp(c.lane, 0, s_lanes - 1);
//    }
//    static void StepTick(EditCursor& c, int d) {
//        int t = c.mbt.tick16 + d;
//        while (t < 0) { t += 4; c.mbt.beat--; }
//        while (t > 3) { t -= 4; c.mbt.beat++; }
//        c.mbt.tick16 = t;
//        while (c.mbt.beat < 0) { c.mbt.beat += 4; c.mbt.measure--; }
//        while (c.mbt.beat > 3) { c.mbt.beat -= 4; c.mbt.measure++; }
//        if (c.mbt.measure < 0) c.mbt.measure = 0;
//        ClampCursor(c);
//    }
//    static void StepBeat(EditCursor& c, int d) {
//        c.mbt.beat += d;
//        while (c.mbt.beat < 0) { c.mbt.beat += 4; c.mbt.measure--; }
//        while (c.mbt.beat > 3) { c.mbt.beat -= 4; c.mbt.measure++; }
//        if (c.mbt.measure < 0) c.mbt.measure = 0;
//        ClampCursor(c);
//    }
//    static void StepMeasure(EditCursor& c, int d) {
//        c.mbt.measure += d;
//        if (c.mbt.measure < 0) c.mbt.measure = 0;
//    }
//
//    // ---- 置く / 消す ----
//    static void PlaceNote(EditScene* scene) {
//        if (!scene) return;
//        NoteData ev{};
//        ev.lane = s_cursor.lane;
//        ev.mbt = s_cursor.mbt;
//        ev.type = s_brush;
//        scene->GetChart().push_back(ev);
//
//        auto& v = scene->GetChart();
//        std::sort(v.begin(), v.end(), [](const NoteData& a, const NoteData& b) {
//            if (a.mbt.measure != b.mbt.measure) return a.mbt.measure < b.mbt.measure;
//            if (a.mbt.beat != b.mbt.beat) return a.mbt.beat < b.mbt.beat;
//            if (a.mbt.tick16 != b.mbt.tick16) return a.mbt.tick16 < b.mbt.tick16;
//            return a.lane < b.lane;
//            });
//    }
//    static void EraseNote(EditScene* scene) {
//        if (!scene) return;
//        auto& v = scene->GetChart();
//        auto it = std::find_if(v.begin(), v.end(), [&](const NoteData& e) {
//            return e.lane == s_cursor.lane
//                && e.mbt.measure == s_cursor.mbt.measure
//                && e.mbt.beat == s_cursor.mbt.beat
//                && e.mbt.tick16 == s_cursor.mbt.tick16;
//            });
//        if (it != v.end()) v.erase(it);
//    }
//
//    // ================= Canvas本体 =================
//    void EditCanvas::Begin() {
//        sf::ui::Canvas::Begin();
//        if (auto* actor = actorRef.Target()) {
//            scene = dynamic_cast<EditScene*>(&actor->GetScene());
//        }
//
//        // カーソル初期化
//        s_cursor = {};
//        s_cursor.lane = 0;
//        s_cursor.mbt = { 0,0,0 };
//        s_brush = NoteType::Tap;
//
//        updateCommand.Bind(std::bind(&EditCanvas::Update, this, std::placeholders::_1));
//
//        // ===== ダミーUI配置 =====
//        // 背景
//        textureBack.LoadTextureFromFile("Assets/Texture/SelectBack.png");
//        backImage = AddUI<sf::ui::Image>();
//        backImage->transform.SetPosition({ 0, 0, 0 });
//        backImage->transform.SetScale({ 20, 15, 0 });
//        backImage->material.texture = &textureBack;
//
//        // 左側パレット（ノーツボタン4つ）
//        textureDummyNote.LoadTextureFromFile("Assets/Texture/note.png");
//        for (int i = 0; i < 4; i++) {
//            auto btn = AddUI<sf::ui::Image>();
//            btn->transform.SetPosition({ -800.0f, 200.0f - i * 120.0f, 0 });
//            btn->transform.SetScale({ 2, 2, 0 });
//            btn->material.texture = &textureDummyNote;
//            dummyNoteButtons.push_back(btn);
//        }
//
//        // 実行ボタン（右下）
//        textureRun.LoadTextureFromFile("Assets/Texture/SelectFrame.png");
//        runButton = AddUI<sf::ui::Image>();
//        runButton->transform.SetPosition({ 200.0f, -300.0f, 0 });
//        runButton->transform.SetScale({ 3, 1.5f, 0 });
//        runButton->material.texture = &textureRun;
//
//        sf::debug::Debug::Log("[EditCanvas] dummy UI placed");
//    }
//
//    void EditCanvas::Update(const sf::command::ICommand&) {
//        auto& I = SInput::Instance();
//
//        // ===== キーボード：カーソル移動 =====
//        if (I.GetKeyDown(Key::KEY_A)) StepTick(s_cursor, -1);
//        if (I.GetKeyDown(Key::KEY_D)) StepTick(s_cursor, +1);
//        if (I.GetKeyDown(Key::KEY_Q)) StepBeat(s_cursor, -1);
//        if (I.GetKeyDown(Key::KEY_E)) StepBeat(s_cursor, +1);
//        if (I.GetKeyDown(Key::KEY_Z)) StepMeasure(s_cursor, -1);
//        if (I.GetKeyDown(Key::KEY_X)) StepMeasure(s_cursor, +1);
//        if (I.GetKeyDown(Key::KEY_W)) { s_cursor.lane--; ClampCursor(s_cursor); }
//        if (I.GetKeyDown(Key::KEY_S)) { s_cursor.lane++; ClampCursor(s_cursor); }
//
//        // ===== ブラシ切替（数字キー）=====
//        if (I.GetKeyDown(Key::KEY_1)) { s_brush = NoteType::Tap;       selectedBrushIdx = 0; }
//        if (I.GetKeyDown(Key::KEY_2)) { s_brush = NoteType::HoldStart; selectedBrushIdx = 1; }
//        if (I.GetKeyDown(Key::KEY_3)) { s_brush = NoteType::HoldEnd;   selectedBrushIdx = 2; }
//        if (I.GetKeyDown(Key::KEY_4)) { s_brush = NoteType::SongEnd;   selectedBrushIdx = 3; }
//
//        // ===== 置く / 消す（キーボード）=====
//        if (I.GetKeyDown(Key::KEY_F)) PlaceNote(scene);
//        if (I.GetKeyDown(Key::KEY_G)) EraseNote(scene);
//
//        // ===== Save / Load =====
//        if (CtrlHeld() && I.GetKeyDown(Key::KEY_S)) {
//            if (scene) scene->SaveChartToFile();
//        }
//        if (CtrlHeld() && I.GetKeyDown(Key::KEY_L)) {
//            if (scene) scene->LoadChartFromFile();
//        }
//
//        // ===== 再生トグル =====
//        if (I.GetKeyDown(Key::SPACE)) {
//            if (scene) {
//                if (scene->IsPlaying()) scene->TransportStop();
//                else                     scene->TransportPlay();
//            }
//        }
//
//        // ===== クリックUI：パレット・実行ボタン =====
//        {
//            const Vector2 mouseUI = GetMousePositionUI();
//
//            // パレット
//            for (int i = 0; i < (int)dummyNoteButtons.size(); ++i) {
//                bool hov = IsHovered(dummyNoteButtons[i], NOTE_BASE_W, NOTE_BASE_H, mouseUI);
//                dummyNoteButtons[i]->transform.SetScale(hov ? Vector3(2.2f, 2.2f, 0) : Vector3(2.0f, 2.0f, 0));
//                if (hov && I.GetMouseDown(0)) {
//                    selectedBrushIdx = i;
//                    switch (i) {
//                    case 0: s_brush = NoteType::Tap;       break;
//                    case 1: s_brush = NoteType::HoldStart; break;
//                    case 2: s_brush = NoteType::HoldEnd;   break;
//                    case 3: s_brush = NoteType::SongEnd;   break;
//                    }
//                }
//            }
//
//            // 実行ボタン
//            if (runButton) {
//                bool hov = IsHovered(runButton, RUN_BASE_W, RUN_BASE_H, mouseUI);
//                runButton->transform.SetScale(hov ? Vector3(3.2f, 1.7f, 0) : Vector3(3.0f, 1.5f, 0));
//                if (hov && I.GetMouseDown(0)) {
//                    if (scene) {
//                        if (scene->IsPlaying()) scene->TransportStop();
//                        else                     scene->TransportPlay();
//                    }
//                }
//            }
//
//            // ===== タイムライン内クリックでノーツ配置 =====
//            if (I.GetMouseDown(0)) {
//                int lane; Mbt16 mbt;
//                if (UiToLaneMBT(mouseUI, lane, mbt)) {
//                    s_cursor.lane = lane;
//                    s_cursor.mbt = mbt;
//                    PlaceNote(scene);
//                }
//            }
//        }
//    }
//
//    // ================= ヘルパ実装 =================
//
//    Vector2 EditCanvas::GetMousePositionUI() {
//        POINT mousePoint;
//        GetCursorPos(&mousePoint);
//        HWND hwnd = GetActiveWindow();
//        ScreenToClient(hwnd, &mousePoint);
//
//        // 画面サイズ：エンジンから取れる関数があれば置換
//        const float screenWidth = 1280.0f;
//        const float screenHeight = 720.0f;
//
//        float uiX = static_cast<float>(mousePoint.x) - screenWidth * 0.5f;
//        float uiY = screenHeight * 0.5f - static_cast<float>(mousePoint.y);
//        return Vector2(uiX, uiY);
//    }
//
//    bool EditCanvas::IsHovered(sf::ui::Image* img, float baseW, float baseH, const Vector2& mouseUI) {
//        if (!img) return false;
//        Vector3 pos = img->transform.GetPosition();
//        Vector3 scale = img->transform.GetScale();
//
//        const float w = baseW * scale.x;
//        const float h = baseH * scale.y;
//
//        const float left = pos.x - w * 0.5f;
//        const float right = pos.x + w * 0.5f;
//        const float bottom = pos.y - h * 0.5f;
//        const float top = pos.y + h * 0.5f;
//
//        return (mouseUI.x >= left && mouseUI.x <= right &&
//            mouseUI.y >= bottom && mouseUI.y <= top);
//    }
//
//    bool EditCanvas::UiToLaneMBT(const Vector2& ui, int& outLane, Mbt16& outMBT) const {
//        // タイムラインの矩形内？
//        if (!(ui.x >= TL_LEFT && ui.x <= TL_RIGHT && ui.y >= TL_BOTTOM && ui.y <= TL_TOP))
//            return false;
//
//        const float tlWidth = TL_RIGHT - TL_LEFT;
//        const float tlHeight = TL_TOP - TL_BOTTOM;
//
//        // lane：上が lane=0 になるように
//        float relY = (ui.y - TL_BOTTOM) / tlHeight;        // 0..1
//        int lane = lanes - 1 - int(relY * lanes);          // 逆順
//        lane = std::clamp(lane, 0, lanes - 1);
//
//        // MBT：X→小節/拍/16分
//        float relX = ui.x - TL_LEFT;                       // 0..width
//        float beatW = pixelsPerBeat;
//        float tickW = beatW / 4.0f;
//
//        int   measure = scrollMeasure + int(relX / (beatW * 4.0f));
//        float remB = std::fmod(relX, beatW * 4.0f);
//        int   beat = int(remB / beatW);
//        float remT = std::fmod(remB, beatW);
//        int   tick16 = int(remT / tickW);
//
//        outLane = lane;
//        outMBT = { measure, beat, tick16 };
//        return true;
//    }
//
//} // namespace app::test

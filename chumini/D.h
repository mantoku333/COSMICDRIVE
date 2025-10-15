#pragma once
#pragma once
#include "Component.h"
#include "DirectWriteCustomFont.h"
#include <memory>

namespace app {
    namespace test {

        class D : public sf::Component {
        public:
            // ライフサイクル
            void Begin() override;                       // 1回だけ初期化
            void Update(const sf::command::ICommand& command); // 毎フレーム描画

            // 画面リサイズ時（SwapChain::ResizeBuffers 後に呼ぶ）
            void OnResize();

            // 設定API
            void SetText(const std::string& text) { mText = text; }
            void SetRect(const D2D1_RECT_F& r) { mRect = r; }
            void SetShadow(bool on) { mShadow = on; }
            void SetColor(const D2D1_COLOR_F& c) { mFont.Color = c; if (mDWrite) mDWrite->SetFont(mFont); }
            void SetFontSize(float px) { mFont.fontSize = px; if (mDWrite) mDWrite->SetFont(mFont); }
            void SetFontByIndex(int idx) { if (mDWrite) { mFont.font = mDWrite->GetFontName(idx); mDWrite->SetFont(mFont); } }

        private:
            std::unique_ptr<DirectWriteCustomFont> mDWrite;
            FontData mFont{};
            std::string mText = "ここはタイトル画面です";
            D2D1_RECT_F mRect = D2D1::RectF(90.f, 90.f, 1000.f, 200.f);
            bool mShadow = true;
        };

    }
} // namespace app::ui

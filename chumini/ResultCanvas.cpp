#include "ResultCanvas.h"
#include "JudgeStatsService.h"
#include "SelectScene.h" 
#include "DirectX11.h"
#include <string>
#include <cstdio>
#include <vector>

namespace app::test {

	void ResultCanvas::Begin() {
		// 基底クラスの初期化
		sf::ui::Canvas::Begin();

		// DirectXデバイスの取得
		auto* dx11 = sf::dx::DirectX11::Instance();
		auto context = dx11->GetMainDevice().GetDevice();


		// =========================================================
		// テキストUIの生成と配置
		// =========================================================

		// --- "RESULT" 
		resultLabel = AddUI<sf::ui::TextImage>();
		resultLabel->transform.SetPosition(Vector3(0, 350, 0));
		resultLabel->transform.SetScale(Vector3(8, 2, 0));
		resultLabel->Create(
			context,
			L"RESULT",
			L"851ゴチカクット",
			100.0f,
			D2D1::ColorF(D2D1::ColorF::Yellow),
			512, 128
		);

		guideText = AddUI<sf::ui::TextImage>();
		guideText->transform.SetPosition(Vector3(0, -400, 0)); 
		guideText->transform.SetScale(Vector3(5, 1.5f, 0));
		guideText->Create(
			context,
			L"---PRESS SPACE---",
			L"851ゴチカクット",
			70.0f,
			D2D1::ColorF(D2D1::ColorF::White),
			800, 128
		);

		// --- 判定内訳
		judgeDetailText = AddUI<sf::ui::TextImage>();
		judgeDetailText->transform.SetPosition(Vector3(-300, -100, 0));
		judgeDetailText->transform.SetScale(Vector3(6, 8, 0));
		judgeDetailText->Create(
			context,
			L"", // 後でセット
			L"851ゴチカクット",
			60.0f,
			D2D1::ColorF(D2D1::ColorF::White),
			512, 1024
		);

		// --- マックスコンボ
		comboText = AddUI<sf::ui::TextImage>();
		comboText->transform.SetPosition(Vector3(-300, -250, 0));
		comboText->transform.SetScale(Vector3(8, 2, 0));
		comboText->Create(
			context,
			L"",
			L"851ゴチカクット",
			70.0f,
			D2D1::ColorF(D2D1::ColorF::Cyan),
			1024, 256
		);

		// --- スコア
		scoreText = AddUI<sf::ui::TextImage>();
		scoreText->transform.SetPosition(Vector3(-300, 100, 0));
		scoreText->transform.SetScale(Vector3(8, 2, 0));
		scoreText->Create(
			context,
			L"",
			L"851ゴチカクット",
			120.0f,
			D2D1::ColorF(D2D1::ColorF::White),
			1024, 256
		);

		// =========================================================
		// 3. ランク表示
		// =========================================================

		// ふちのズレ幅（太さ）
		float outlineOffset = 4.0f;
		Vector3 offsets[4] = {
			Vector3(-outlineOffset, 0, 0), // 左
			Vector3(outlineOffset, 0, 0), // 右
			Vector3(0, -outlineOffset, 0), // 下
			Vector3(0,  outlineOffset, 0)  // 上
		};

		// --- ふち文字 (奥に配置) ---
		for (int i = 0; i < 4; ++i) {
			rankOutline[i] = AddUI<sf::ui::TextImage>();
			// 少しズラし、Z軸を奥(0.1f)にする
			rankOutline[i]->transform.SetPosition(Vector3(450 + offsets[i].x, -100 + offsets[i].y, 0.1f));
			rankOutline[i]->transform.SetScale(Vector3(20, 20, 0)); // 巨大サイズ
			rankOutline[i]->Create(
				context,
				L"", // 後でランク文字を入れる
				L"851ゴチカクット",
				150.0f,
				D2D1::ColorF(D2D1::ColorF::Black), // 初期色は黒
				512, 512
			);
		}

		// --- メイン文字 (手前に配置) ---
		rankText = AddUI<sf::ui::TextImage>();
		rankText->transform.SetPosition(Vector3(450, -100, 0));
		rankText->transform.SetScale(Vector3(20, 20, 0));
		rankText->Create(
			context,
			L"",
			L"851ゴチカクット",
			150.0f,
			D2D1::ColorF(D2D1::ColorF::White),
			512, 512
		);

		// =========================================================
		// 4. スコア計算 & ランク判定ロジック
		// =========================================================
		int perfect = JudgeStatsService::GetCount(JudgeResult::Perfect);
		int great = JudgeStatsService::GetCount(JudgeResult::Great);
		int good = JudgeStatsService::GetCount(JudgeResult::Good);
		int miss = JudgeStatsService::GetCount(JudgeResult::Miss);
		int maxCombo = JudgeStatsService::GetMaxCombo();

		// 簡易スコア計算 (満点 1,000,000 点)
		int totalNotes = perfect + great + good + miss;
		int score = 0;
		if (totalNotes > 0) {
			// 配点: Perfect=1.0, Great=0.8, Good=0.5, Miss=0.0
			double p = (perfect * 1.0 + great * 0.8 + good * 0.5) / totalNotes;
			score = static_cast<int>(p * 1000000);
		}

		// --- ランクと色の決定 ---
		std::wstring rankStr = L"C";

		// デフォルト（Cランク）は緑
		D2D1_COLOR_F mainColor = D2D1::ColorF(0.8f, 1.0f, 0.8f); // 薄い緑
		D2D1_COLOR_F edgeColor = D2D1::ColorF(0.0f, 0.8f, 0.0f); // 濃い緑

		if (score >= 1000000) {
			rankStr = L"SSS";
			// 金 (豪華)
			mainColor = D2D1::ColorF(1.0f, 1.0f, 0.9f); // キラキラした白金
			edgeColor = D2D1::ColorF(1.0f, 0.85f, 0.0f); // 純金
		}
		else if (score >= 900000) {
			rankStr = L"SS";
			// 金
			mainColor = D2D1::ColorF(1.0f, 1.0f, 0.6f);
			edgeColor = D2D1::ColorF(0.85f, 0.7f, 0.0f);
		}
		else if (score >= 800000) {
			rankStr = L"S";
			// 金 (スタンダード)
			mainColor = D2D1::ColorF(1.0f, 0.95f, 0.4f);
			edgeColor = D2D1::ColorF(0.8f, 0.6f, 0.0f);
		}
		else if (score >= 600000) {
			rankStr = L"A";
			// 赤
			mainColor = D2D1::ColorF(1.0f, 0.8f, 0.8f); // 薄い赤
			edgeColor = D2D1::ColorF(1.0f, 0.0f, 0.0f); // 真っ赤
		}
		else if (score >= 400000) {
			rankStr = L"B";
			// 青
			mainColor = D2D1::ColorF(0.8f, 0.9f, 1.0f); // 薄い青
			edgeColor = D2D1::ColorF(0.0f, 0.0f, 1.0f); // 純青
		}
		else {
			rankStr = L"C";
			// 緑
			mainColor = D2D1::ColorF(0.8f, 1.0f, 0.8f); // 薄い緑
			edgeColor = D2D1::ColorF(0.0f, 0.7f, 0.0f); // 濃い緑
		}

		// =========================================================
		// 5. テキストへの値セット
		// =========================================================

		// 判定詳細
		wchar_t detailBuf[256];
		swprintf_s(detailBuf,
			L"PERFECT: %d\n"
			L"GREAT:   %d\n"
			L"GOOD:    %d\n"
			L"MISS:    %d",
			perfect, great, good, miss);
		judgeDetailText->SetText(detailBuf);

		// マックスコンボ
		wchar_t comboBuf[64];
		swprintf_s(comboBuf, L"MAX COMBO: %d", maxCombo);
		comboText->SetText(comboBuf);

		// スコア
		wchar_t scoreBuf[64];
		swprintf_s(scoreBuf, L"SCORE: %d", score);
		scoreText->SetText(scoreBuf);

		// ランク (本体)
		rankText->SetText(rankStr);
		rankText->material.SetColor({ mainColor.r, mainColor.g, mainColor.b, 1.0f });

		// ランク (ふち)
		for (int i = 0; i < 4; ++i) {
			if (rankOutline[i]) {
				rankOutline[i]->SetText(rankStr);
				rankOutline[i]->material.SetColor({ edgeColor.r, edgeColor.g, edgeColor.b, 1.0f });
			}
		}

		// =========================================================
		// 6. シーン遷移準備
		// =========================================================
		if (nextScene.isNull()) {
			nextScene = SelectScene::StandbyScene();
		}

		updateCommand.Bind(std::bind(&ResultCanvas::Update, this, std::placeholders::_1));
	}

	void ResultCanvas::Update(const sf::command::ICommand&) {

		// 点滅アニメーション
		timer += sf::Time::DeltaTime();
		if (guideText) {
			float alpha = 0.65f + 0.35f * std::sin(timer * 3.0f);
			guideText->material.SetColor({ 1.0f, 1.0f, 1.0f, alpha });
		}

		// スペースキーでセレクト画面へ戻る
		if (SInput::Instance().GetKeyDown(Key::SPACE)) {
			if (nextScene->StandbyThisScene()) {
				nextScene->Activate();

				auto owner = actorRef.Target();
				if (owner) owner->GetScene().DeActivate();
			}
		}
	}

} // namespace app::test
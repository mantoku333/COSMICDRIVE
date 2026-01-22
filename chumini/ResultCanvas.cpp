#include "ResultCanvas.h"
#include "JudgeStatsService.h"
#include "SelectScene.h" 
#include "DirectX11.h"
#include <string>
#include <cstdio>
#include <vector>
#include "ScoreManager.h"

namespace app::test {

	void ResultCanvas::Begin() {
		// 基底クラスの初期化
		sf::ui::Canvas::Begin();

		// DirectXデバイスの取得
		auto* dx11 = sf::dx::DirectX11::Instance();
		auto context = dx11->GetMainDevice().GetDevice();


		// SceneChangeComponent取得
		if (auto actor = actorRef.Target()) {
			auto* changer = actor->GetComponent<SceneChangeComponent>();
			if (changer) {
				sceneChanger = changer;
			}
		}

		// =========================================================
		// テキストUIの生成と配置
		// =========================================================

		// --- RESULTラベル ---
		resultLabel = AddUI<sf::ui::TextImage>();
		resultLabel->transform.SetPosition(Vector3(0, 380, 0));
		resultLabel->transform.SetScale(Vector3(8, 2, 0));
		resultLabel->Create(
			context,
			L"RESULT",
			L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
			100.0f,
			D2D1::ColorF(D2D1::ColorF::Yellow),
			512, 128
		);

		// --- 操作ガイド
		guideText = AddUI<sf::ui::TextImage>();
		guideText->transform.SetPosition(Vector3(0, -450, 0)); 
		guideText->transform.SetScale(Vector3(5, 1.5f, 0));
		guideText->Create(
			context,
			L"---PRESS SPACE---",
			L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
			70.0f,
			D2D1::ColorF(D2D1::ColorF::White),
			800, 128
		);

		// --- 判定内訳 (PERFECT)
		perfectText = AddUI<sf::ui::TextImage>();
		perfectText->transform.SetPosition(Vector3(-350, 80, 0));
		perfectText->transform.SetScale(Vector3(9.0f, 1.5f, 0));   // 横長・縦短
		perfectText->Create(
			context, L"", 
			L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf", 
			60.0f, D2D1::ColorF(D2D1::ColorF::Yellow), 1024, 128
		);

		// --- 判定内訳 (GREAT)
		greatText = AddUI<sf::ui::TextImage>();
		greatText->transform.SetPosition(Vector3(-350, 10, 0));
		greatText->transform.SetScale(Vector3(9.0f, 1.5f, 0));
		greatText->Create(
			context, L"",
			L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
			60.0f, D2D1::ColorF(D2D1::ColorF::HotPink), 1024, 128
		);

		// --- 判定内訳 (GOOD)
		goodText = AddUI<sf::ui::TextImage>();
		goodText->transform.SetPosition(Vector3(-350, -60, 0));
		goodText->transform.SetScale(Vector3(9.0f, 1.5f, 0));
		goodText->Create(
			context, L"",
			L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
			60.0f, D2D1::ColorF(D2D1::ColorF::LimeGreen), 1024, 128
		);

		// --- 判定内訳 (MISS)
		missText = AddUI<sf::ui::TextImage>();
		missText->transform.SetPosition(Vector3(-350, -130, 0));
		missText->transform.SetScale(Vector3(9.0f, 1.5f, 0));
		missText->Create(
			context, L"",
			L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
			60.0f, D2D1::ColorF(D2D1::ColorF::Gray), 1024, 128
		);

		// --- 判定内訳 (FAST)
		// MISSとの間を少し空ける
		fastText = AddUI<sf::ui::TextImage>();
		fastText->transform.SetPosition(Vector3(-500, -230, 0));
		fastText->transform.SetScale(Vector3(7.0f, 1.2f, 0));
		fastText->Create(
			context, L"",
			L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
			60.0f, D2D1::ColorF(D2D1::ColorF::OrangeRed), 1024, 128
		);

		// --- 判定内訳 (SLOW)
		slowText = AddUI<sf::ui::TextImage>();
		slowText->transform.SetPosition(Vector3(-200, -230, 0));
		slowText->transform.SetScale(Vector3(7.0f, 1.2f, 0));
		slowText->Create(
			context, L"",
			L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
			60.0f, D2D1::ColorF(D2D1::ColorF::Cyan), 1024, 128
		);


		// --- マックスコンボ
		comboText = AddUI<sf::ui::TextImage>();
		comboText->transform.SetPosition(Vector3(-350, -330, 0)); // -360 -> -290 -> -330
		comboText->transform.SetScale(Vector3(8, 2, 0));
		comboText->Create(
			context,
			L"",
			L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
			70.0f,
			D2D1::ColorF(D2D1::ColorF::Cyan),
			1024, 256
		);

		// --- スコア
		// ... existing score code ... (Assuming this part is outside the target content range if I target correctly, but I need to include it if it moves relative to others)
        // Ignoring scoreText update in Replace Content since I only select the Judge Detail part.
        
        // Wait, I should make sure I don't delete scoreText init if I select a large block.
        // Let's stick to the JudgeDetail part.
        
        // Actually, the previous tool call set judgeDetailText then comboText.
        // I will replace from judgeDetailText definition up to comboText definition.
        
        // Wait, I need to look at the line numbers again to be safe.
        // judgeDetailText is Lines 59-69
        // comboText is Lines 72-82
        // scoreText is Lines 85-95
        
        // I will replace Lines 59-82 (Judge + Combo)

        // =========================================================
		// 3. ランク表示
		// =========================================================

		// ...
		
		// =========================================================
		// 5. テキストへの値セット
		// =========================================================

        // ... (Update this part too)



		// --- スコア
		scoreText = AddUI<sf::ui::TextImage>();
		scoreText->transform.SetPosition(Vector3(-350, 180, 0)); // 上すぎたので下げる
		scoreText->transform.SetScale(Vector3(12, 3, 0));
		scoreText->Create(
			context,
			L"",
			L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
			120.0f,
			D2D1::ColorF(D2D1::ColorF::White),
			2048, 256
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
			rankOutline[i]->transform.SetPosition(Vector3(450 + offsets[i].x, 100 + offsets[i].y, 0.1f));
			rankOutline[i]->transform.SetScale(Vector3(13, 13, 0)); // 巨大サイズ
			rankOutline[i]->Create(
				context,
				L"", // 後でランク文字を入れる
				L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
				150.0f,
				D2D1::ColorF(D2D1::ColorF::White), // 初期色は白 (マテリアル色で着色するため)
				512, 512
			);
		}

		// --- メイン文字 (手前に配置) ---
		rankText = AddUI<sf::ui::TextImage>();
		rankText->transform.SetPosition(Vector3(450, 100, 0));
		rankText->transform.SetScale(Vector3(13, 13, 0));
		rankText->Create(
			context,
			L"",
			L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
			134.0f,
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
		int fast = JudgeStatsService::GetFastCount();
		int slow = JudgeStatsService::GetSlowCount();
		int maxCombo = JudgeStatsService::GetMaxCombo();

		// 簡易スコア計算 (満点 1,000,000 点)
		int totalNotes = perfect + great + good + miss;
		int score = 0;
		if (totalNotes > 0) {
			// 配点: Perfect=1.0, Great=0.8, Good=0.5, Miss=0.0
			double p = (perfect * 1.0 + great * 0.8 + good * 0.5) / totalNotes;
			score = static_cast<int>(p * 1000000);
		}
		
		targetScore = score;
		displayScore = 0.0f;
		isScoreAnimationFinished = false;

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

        // --- Save Score ---
        std::string chartPath = JudgeStatsService::GetChartPath();
        if (!chartPath.empty()) {
            std::string rankUtf8(rankStr.begin(), rankStr.end()); // ASCII only
            app::test::ScoreManager::Instance().UpdateScore(chartPath, score, rankUtf8);
            app::test::ScoreManager::Instance().Save();
            sf::debug::Debug::Log("Score Saved: " + std::to_string(score) + " Rank: " + rankUtf8);
        } else {
             sf::debug::Debug::Log("Score Check: ChartPath is empty, not saved.");
        }

		// =========================================================
		// 5. テキストへの値セット
		// =========================================================

		// 判定詳細
		wchar_t buf[64];
		
		if (perfectText) {
			swprintf_s(buf, L"PERFECT: %d", perfect);
			perfectText->SetText(buf);
		}
		if (greatText) {
			swprintf_s(buf, L"GREAT:   %d", great);
			greatText->SetText(buf);
		}
		if (goodText) {
			swprintf_s(buf, L"GOOD:    %d", good);
			goodText->SetText(buf);
		}
		if (missText) {
			swprintf_s(buf, L"MISS:    %d", miss);
			missText->SetText(buf);
		}
		if (fastText) {
			swprintf_s(buf, L"FAST:    %d", fast);
			fastText->SetText(buf);
		}
		if (slowText) {
			swprintf_s(buf, L"SLOW:    %d", slow);
			slowText->SetText(buf);
		}

		// マックスコンボ
		wchar_t comboBuf[64];
		swprintf_s(comboBuf, L"MAX COMBO: %d", maxCombo);
		comboText->SetText(comboBuf);



		// スコア
		wchar_t scoreBuf[64];
		swprintf_s(scoreBuf, L"SCORE: %d", 0); // スタートは0点から
		scoreText->SetText(scoreBuf);

		// ランク (初期表示は0点相当)
		UpdateRankDisplay(0);

		updateCommand.Bind(std::bind(&ResultCanvas::Update, this, std::placeholders::_1));
	}

	void ResultCanvas::Update(const sf::command::ICommand&) {

		// スコアアニメーション
		if (!isScoreAnimationFinished && scoreText) {
			float dt = sf::Time::DeltaTime();
			float diff = (float)targetScore - displayScore;
			
			// 差分が小さくなったら完了
			if (std::abs(diff) < 1.0f) {
				displayScore = (float)targetScore;
				isScoreAnimationFinished = true;
			} else {
				// ターゲットに近づくように補間 (係数5.0fはスピード調整用)
				displayScore += diff * 4.0f * dt;
			}

			wchar_t scoreBuf[64];
			swprintf_s(scoreBuf, L"SCORE: %d", (int)displayScore);
			scoreText->SetText(scoreBuf);
			
			// ランクもアニメーション
			UpdateRankDisplay((int)displayScore);
		}

		// 点滅アニメーション
		timer += sf::Time::DeltaTime();
		if (guideText) {
			float alpha = 0.65f + 0.35f * std::sin(timer * 3.0f);
			guideText->material.SetColor({ 1.0f, 1.0f, 1.0f, alpha });
		}

		// スペースキーでセレクト画面へ戻る
		if (SInput::Instance().GetKeyDown(Key::SPACE)) {
			// ★リファクタリングポイント：isNull() でチェックして丸投げ！
			if (!sceneChanger.isNull()) {
				sf::debug::Debug::Log("Selectへ遷移");

				// 遷移先のシーンを生成して渡すだけ。あとの面倒はコンポーネントがみます。
				sceneChanger->ChangeScene(SelectScene::StandbyScene());
			}
		}
	}

	void ResultCanvas::UpdateRankDisplay(int score) {
		std::wstring rankStr = L"C";
		D2D1_COLOR_F mainColor = D2D1::ColorF(0.8f, 1.0f, 0.8f);
		D2D1_COLOR_F edgeColor = D2D1::ColorF(0.0f, 0.8f, 0.0f);

		if (score >= 1000000) {
			rankStr = L"SSS";
			mainColor = D2D1::ColorF(1.0f, 1.0f, 0.9f);
			edgeColor = D2D1::ColorF(1.0f, 0.85f, 0.0f);
		}
		else if (score >= 900000) {
			rankStr = L"SS";
			mainColor = D2D1::ColorF(1.0f, 1.0f, 0.6f);
			edgeColor = D2D1::ColorF(0.85f, 0.7f, 0.0f);
		}
		else if (score >= 800000) {
			rankStr = L"S";
			mainColor = D2D1::ColorF(1.0f, 0.95f, 0.4f);
			edgeColor = D2D1::ColorF(0.8f, 0.6f, 0.0f);
		}
		else if (score >= 600000) {
			rankStr = L"A";
			mainColor = D2D1::ColorF(1.0f, 0.8f, 0.8f);
			edgeColor = D2D1::ColorF(1.0f, 0.0f, 0.0f);
		}
		else if (score >= 400000) {
			rankStr = L"B";
			mainColor = D2D1::ColorF(0.8f, 0.9f, 1.0f);
			edgeColor = D2D1::ColorF(0.0f, 0.0f, 1.0f);
		}
		else {
			rankStr = L"C";
			mainColor = D2D1::ColorF(0.8f, 1.0f, 0.8f);
			edgeColor = D2D1::ColorF(0.0f, 0.7f, 0.0f);
		}

		if (rankText) {
			rankText->SetText(rankStr);
			rankText->material.SetColor({ mainColor.r, mainColor.g, mainColor.b, 1.0f });
		}
		for (int i = 0; i < 4; ++i) {
			if (rankOutline[i]) {
				rankOutline[i]->SetText(rankStr);
				rankOutline[i]->material.SetColor({ edgeColor.r, edgeColor.g, edgeColor.b, 1.0f });
			}
		}
	}

} // namespace app::test
#include "IngameCanvas.h"
#include <sstream>
#include <iomanip>
#include "Time.h"
#include "NoteManager.h"
#include "GameSession.h"
#include <algorithm>
#include "DirectX11.h"
#include "StringUtils.h"  // 文字コード変換ユーティリティ

namespace app::test {

	void IngameCanvas::Begin()
	{
		sf::ui::Canvas::Begin();

		auto* dx11 = sf::dx::DirectX11::Instance();
		auto context = dx11->GetMainDevice().GetDevice();

		// =========================================================
		// 繝・け繧ｹ繝√Ε隱ｭ縺ｿ霎ｼ縺ｿ
		// =========================================================
		textureJacket.LoadTextureFromFile("Assets\\Texture\\GOODTEK.png");
		textureDefaultJacket.LoadTextureFromFile("Assets\\Texture\\DefaultJacket.png");


		if (!textureHitEffect.LoadTextureFromFile("Assets\\Texture\\Effect-Tap.png")) {
			sf::debug::Debug::LogError("IngameCanvas: Failed to load Effect-Tap.png");
		}
        if (!textureSlashEffect.LoadTextureFromFile("Assets\\Texture\\Slash.png")) {
            sf::debug::Debug::LogError("IngameCanvas: Failed to load Slash.png");
        }

		// =========================================================
		// UI逕滓・
		// =========================================================
		Jacket = AddUI<sf::ui::Image>();
		Jacket->transform.SetPosition(Vector3(810, 400, 0));
		Jacket->transform.SetScale(Vector3(3, 3, 0));
		Jacket->material.texture = &textureJacket;




		// ---------------------------------------------------------
		// 繝・く繧ｹ繝・I逕滓・
		// ---------------------------------------------------------


		// 2. 繧ｳ繝ｳ繝・
		comboText = AddUI<sf::ui::TextImage>();
		comboText->transform.SetPosition(Vector3(650, -100, 0)); // Raised by 50 (-150 -> -100)
		comboText->transform.SetScale(Vector3(4.5f, 1.8f, 0));
		if (!comboText->Create(context, L"", L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf", 100.0f, D2D1::ColorF(D2D1::ColorF::Cyan), 512, 256)) {
			sf::debug::Debug::LogError("IngameCanvas: Failed to create comboText");
		}

		// 3. 判定内訳 (個別表示)
		// 左端 (-960 ~ )。少し右に寄せて -700 付近
		// Y座標は調整

		// PERFECT
		perfectText = AddUI<sf::ui::TextImage>();
		perfectText->transform.SetPosition(Vector3(-700, 0, 0)); // Raised by 50 (-50 -> 0)
		perfectText->transform.SetScale(Vector3(3.5f, 1.5f, 0)); 
		if (!perfectText->Create(context, L"PERFECT: 0", L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf", 50.0f, D2D1::ColorF(D2D1::ColorF::Yellow), 512, 128)) {
			sf::debug::Debug::LogError("IngameCanvas: Failed to create perfectText");
		}

		// GREAT
		greatText = AddUI<sf::ui::TextImage>();
		greatText->transform.SetPosition(Vector3(-700, -50, 0)); // Raised by 50 (-100 -> -50)
		greatText->transform.SetScale(Vector3(3.5f, 1.5f, 0));
		greatText->Create(context, L"GREAT: 0", L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf", 50.0f, D2D1::ColorF(D2D1::ColorF::HotPink), 512, 128);

		// GOOD
		goodText = AddUI<sf::ui::TextImage>();
		goodText->transform.SetPosition(Vector3(-700, -100, 0)); // Raised by 50 (-150 -> -100)
		goodText->transform.SetScale(Vector3(3.5f, 1.5f, 0));
		goodText->Create(context, L"GOOD: 0", L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf", 50.0f, D2D1::ColorF(D2D1::ColorF::LimeGreen), 512, 128);

		// MISS
		missText = AddUI<sf::ui::TextImage>();
		missText->transform.SetPosition(Vector3(-700, -150, 0)); // Raised by 50 (-200 -> -150)
		missText->transform.SetScale(Vector3(3.5f, 1.5f, 0));
		missText->Create(context, L"MISS: 0", L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf", 50.0f, D2D1::ColorF(D2D1::ColorF::Gray), 512, 128);


		// 4. 判定結果表示
		judgeResultText = AddUI<sf::ui::TextImage>();
		judgeResultText->transform.SetPosition(Vector3(0, -200, 0)); // Raised by 50 (-250 -> -200)
		judgeResultText->transform.SetScale(Vector3(4.5f, 1.15f, 0));
		if (!judgeResultText->Create(
			context,
			L"", // 最初は空
			L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
			120.0f,
			D2D1::ColorF(D2D1::ColorF::White),
			1024, 256
		)) {
			sf::debug::Debug::LogError("IngameCanvas: Failed to create judgeResultText");
		}

		// FAST/SLOW
		fastSlowText = AddUI<sf::ui::TextImage>();
		fastSlowText->transform.SetPosition(Vector3(0, -250, 0)); // Raised by 50 (-300 -> -250)
		fastSlowText->transform.SetScale(Vector3(1.5f, 0.5f, 0));
		if (!fastSlowText->Create(context, L"", L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf", 80.0f, D2D1::ColorF(D2D1::ColorF::White), 512, 128)) {
			sf::debug::Debug::LogError("IngameCanvas: Failed to create fastSlowText");
		}

		// 5. カウントダウン用
		countdownText = AddUI<sf::ui::TextImage>();
		countdownText->transform.SetPosition(Vector3(0, 100, 0));
		countdownText->transform.SetScale(Vector3(10, 5, 0)); // 初期サイズ
		countdownText->Create(
			context,
			L"",
			L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf",
			120.0f,
			D2D1::ColorF(D2D1::ColorF::Yellow),
			1024, 256
		);

        // Score (Top Left)
        scoreText = AddUI<sf::ui::TextImage>();
        scoreText->transform.SetPosition(Vector3(-750, 450, 0)); // Top Left
        scoreText->transform.SetScale(Vector3(3.5f, 1.5f, 0)); 
		if (!scoreText->Create(context, L"0000000", L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf", 60.0f, D2D1::ColorF(D2D1::ColorF::White), 512, 128)) {
			sf::debug::Debug::LogError("IngameCanvas: Failed to create scoreText");
		}

        // Rank (Next to Score)
        rankText = AddUI<sf::ui::TextImage>();
        rankText->transform.SetPosition(Vector3(-550, 450, 0)); // Right of Score
        rankText->transform.SetScale(Vector3(2.5f, 1.5f, 0)); 
		if (!rankText->Create(context, L"D", L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf", 60.0f, D2D1::ColorF(D2D1::ColorF::Gray), 512, 128)) {
			sf::debug::Debug::LogError("IngameCanvas: Failed to create rankText");
		}




        // Rank Markers (B, A, S)
        const wchar_t* markerChars[] = { L"B", L"A", L"S" };
        for (int i = 0; i < 3; i++) {
            rankLabels[i] = AddUI<sf::ui::TextImage>();
            rankLabels[i]->transform.SetScale(Vector3(0.7f, 0.35f, 0)); // Smaller
            // Position will be set in DrawScoreGauge
            rankLabels[i]->Create(context, markerChars[i], L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf", 50.0f, D2D1::ColorF(D2D1::ColorF::White), 128, 64);
        }

        // Song Info (Below Jacket)
        // Jacket is at (810, 400). Assuming Scale(3,3) makes it reasonably large.
        // Placing text below.
        
        // Title
        titleText = AddUI<sf::ui::TextImage>();
        titleText->transform.SetPosition(Vector3(810, 180, 0)); 
        titleText->transform.SetScale(Vector3(3.5f, 0.6f, 0)); // Smaller
        titleText->Create(context, L"", L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf", 60.0f, D2D1::ColorF(D2D1::ColorF::White), 1024, 128); // Width 1024

        // BPM
        bpmText = AddUI<sf::ui::TextImage>();
        bpmText->transform.SetPosition(Vector3(810, 130, 0));
        bpmText->transform.SetScale(Vector3(2.5f, 0.5f, 0)); // Smaller
        bpmText->Create(context, L"", L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf", 50.0f, D2D1::ColorF(D2D1::ColorF::White), 512, 128); // Width 512

        // Difficulty
        difficultyText = AddUI<sf::ui::TextImage>();
        difficultyText->transform.SetPosition(Vector3(810, 80, 0));
        difficultyText->transform.SetScale(Vector3(2.5f, 0.5f, 0)); // Smaller
        difficultyText->Create(context, L"", L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf", 50.0f, D2D1::ColorF(D2D1::ColorF::White), 512, 128); // Width 512

		// =========================================================
		// 初期化
		// =========================================================
		if (auto actor = actorRef.Target()) {
			if (auto manager = actor->GetComponent<EffectManager>()) {
				effectManager = manager;
				effectManager->InitializeSprite(
					[this]() { return this->AddUI<sf::ui::Image>(); },
					&textureHitEffect,
					20
				);
			}
            // Slash Effect Manager (Add Component)
            auto slashMgrData = actor->AddComponent<EffectManager>();
            slashEffectManager = slashMgrData;
            
            if (!slashEffectManager.isNull()) {
                // Slash Texture Layout: 5x8 (Cols=5, Rows=8)
                slashEffectManager->SetGrid(5, 8);
                
                slashEffectManager->InitializeSprite(
                    [this]() { return this->AddUI<sf::ui::Image>(); },
                    &textureSlashEffect,
                    20 // Pool size
                );
            }

			noteManager = actor->GetComponent<NoteManager>();
		}


		UpdateJacketImage();
		
		// カウントダウン状態初期化
		lastCountdownVal = -1;
		isCountdownStartShown = false;

		updateCommand.Bind(std::bind(&IngameCanvas::Update, this, std::placeholders::_1));
	}

	void IngameCanvas::Update(const sf::command::ICommand&)
	{
		countUpTimer += sf::Time::DeltaTime();



		int combo = GetCurrentSession().GetCount(JudgeResult::Perfect) + GetCurrentSession().GetCount(JudgeResult::Great) + GetCurrentSession().GetCount(JudgeResult::Good); // 簡易計算 or GetCurrentSession().GetCombo() if available

		// ... (コンボ制御はあとで)
		// GetCurrentSession().GetCombo() があればそれを使う
		combo = GetCurrentSession().GetCombo();

		// コンボ変動時の初期化/脈動判定
		if (lastCombo == -1) {
			lastCombo = combo;
		}
		else if (combo != lastCombo) {
			if (combo > lastCombo) {
				// コンボが増えたときだけ脈動させる。より俊敏に（0.1秒）
				comboScaleTimer = 0.10f; 
			}
			lastCombo = combo;
		}

		// コンボ脈動アニメーション
		float scaleRate = 1.0f;
		if (comboScaleTimer > 0.0f) {
			comboScaleTimer -= sf::Time::DeltaTime();
			if (comboScaleTimer < 0.0f) comboScaleTimer = 0.0f;
			
			// 単純な線形補間で拡大率を決める（必要ならEasingを入れる）
			// 最大1.6倍くらいまで大きくする
			float t = comboScaleTimer / 0.10f; // 1.0 -> 0.0
			scaleRate = 1.0f + t * 0.6f;
		}

		if (comboText) {
			// 定数：初期スケール (4.5, 1.8, 0)
			comboText->transform.SetScale(Vector3(4.5f * scaleRate, 1.8f * scaleRate, 0.0f));
		}

		UpdateComboDisplay(combo);
		UpdateJudgeCountDisplay();

		// Updateでの判定結果更新
		JudgeResult currentJudge = GetCurrentSession().GetLastResult();
		if (currentJudge != lastJudgeResult) {
			SetJudgeImage(currentJudge);
			lastJudgeResult = currentJudge;
		}

		// 判定脈動ロジック
		// カウントダウン中は適用しない
		if (!isCountdownActive) {
			int currentTotalJudges = GetCurrentSession().GetCount(JudgeResult::Perfect)
				+ GetCurrentSession().GetCount(JudgeResult::Great)
				+ GetCurrentSession().GetCount(JudgeResult::Good)
				+ GetCurrentSession().GetCount(JudgeResult::Miss);

			if (lastTotalJudges == -1) {
				lastTotalJudges = currentTotalJudges;
			}
			else if (currentTotalJudges > lastTotalJudges) {
				judgeScaleTimer = 0.10f; // 脈動開始
				lastTotalJudges = currentTotalJudges;
			}

			float judgeScaleRate = 1.0f;
			if (judgeScaleTimer > 0.0f) {
				judgeScaleTimer -= sf::Time::DeltaTime();
				if (judgeScaleTimer < 0.0f) judgeScaleTimer = 0.0f;

				float t = judgeScaleTimer / 0.10f;
				judgeScaleRate = 1.0f + t * 0.5f; // Combox0.6に近い0.5倍拡大
			}

			if (judgeResultText) {
				// Base Scale (4.5, 1.15)
				judgeResultText->transform.SetScale(Vector3(4.5f * judgeScaleRate, 1.15f * judgeScaleRate, 0.0f));
			}
		}

		// ---------------------------------------------------------
		// FAST/SLOW 脈動ロジック
		// ---------------------------------------------------------
		float fsScaleRate = 1.0f;
		if (fastSlowScaleTimer > 0.0f) {
			fastSlowScaleTimer -= sf::Time::DeltaTime();
			if (fastSlowScaleTimer < 0.0f) fastSlowScaleTimer = 0.0f;

			float t = fastSlowScaleTimer / 0.10f;
			fsScaleRate = 1.0f + t * 0.5f; // Combox0.6に近い0.5倍拡大
		}

		if (fastSlowText) {
			// Base Scale (1.5, 0.5)
			fastSlowText->transform.SetScale(Vector3(1.5f * fsScaleRate, 0.5f * fsScaleRate, 0.0f));
		}

        UpdateScoreDisplay();
	}

    void IngameCanvas::UpdateScoreDisplay() {
        if (!scoreText || !rankText || !noteManager) return;

        int maxCombo = noteManager->GetMaxTotalCombo();
        if (maxCombo <= 0) maxCombo = 1; // Safety

        // Current weighted sum
        int perfect = GetCurrentSession().GetCount(JudgeResult::Perfect);
        int great = GetCurrentSession().GetCount(JudgeResult::Great);
        int good = GetCurrentSession().GetCount(JudgeResult::Good);
        // Miss doesn't add score

        // Calculation: (CurrentSum / MaxPossibleSum) * 1000000
        double currentSum = perfect * 1.0 + great * 0.8 + good * 0.5;
        // Assume MaxPossible is MaxCombo * 1.0
        double maxPossible = (double)maxCombo;
        
        int score = (int)((currentSum / maxPossible) * 1000000.0);
        if (score > 1000000) score = 1000000; // Cap?

        // Rank (Synced with ResultCanvas)
        std::wstring rankStr = L"C";
        DirectX::XMFLOAT4 rankColor = { 0.8f, 1.0f, 0.8f, 1.0f }; // C (Green)

        if (score >= 1000000) { rankStr = L"SSS"; rankColor = { 1.0f, 1.0f, 0.9f, 1.0f }; }
        else if (score >= 900000) { rankStr = L"SS"; rankColor = { 1.0f, 1.0f, 0.6f, 1.0f }; }
        else if (score >= 800000) { rankStr = L"S"; rankColor = { 1.0f, 0.95f, 0.4f, 1.0f }; }
        else if (score >= 600000) { rankStr = L"A"; rankColor = { 1.0f, 0.8f, 0.8f, 1.0f }; }
        else if (score >= 400000) { rankStr = L"B"; rankColor = { 0.8f, 0.9f, 1.0f, 1.0f }; }
        // Else C

        wchar_t buf[32];
        swprintf_s(buf, L"%07d", score);
        scoreText->SetText(buf);

        rankText->SetText(rankStr.c_str());
        rankText->material.SetColor(rankColor);
    }

	void IngameCanvas::SpawnHitEffect(float x, float y, float scale, float duration, const Color& color)
	{
		if (!effectManager.isNull()) {
			// 笘・せ繝励Λ繧､繝亥・逕溘・縺ｿ
			effectManager->SpawnSprite(x, y, scale, duration, color);
		}
	}

    void IngameCanvas::SpawnSlashEffect(float x, float y, float scaleX, float scaleY, float duration, const Color& color)
    {
        if (!slashEffectManager.isNull()) {
            slashEffectManager->SpawnSprite(x, y, scaleX, scaleY, duration, color);
        }
    }

	// ---------------------------------------------------------
	// 陦ｨ遉ｺ譖ｴ譁ｰ髢｢謨ｰ
	// ---------------------------------------------------------



	void IngameCanvas::UpdateComboDisplay(int combo)
	{
		if (!comboText) return;
		if (combo > 0) {
			wchar_t buf[32];
			swprintf_s(buf, L"%d COMBO", combo);
			comboText->SetText(buf);
		}
		else {
			comboText->SetText(L"");
		}
	}

	void IngameCanvas::UpdateJudgeCountDisplay()
	{
		int p = GetCurrentSession().GetCount(JudgeResult::Perfect);
		int gr = GetCurrentSession().GetCount(JudgeResult::Great);
		int go = GetCurrentSession().GetCount(JudgeResult::Good);
		int m = GetCurrentSession().GetCount(JudgeResult::Miss);

		wchar_t buf[64];
		
		if (perfectText) {
			swprintf_s(buf, L"PERFECT: %d", p);
			perfectText->SetText(buf);
		}
		if (greatText) {
			swprintf_s(buf, L"GREAT:   %d", gr);
			greatText->SetText(buf);
		}
		if (goodText) {
			swprintf_s(buf, L"GOOD:    %d", go);
			goodText->SetText(buf);
		}
		if (missText) {
			swprintf_s(buf, L"MISS:    %d", m);
			missText->SetText(buf);
		}
	}

	// 笘・未謨ｰ蜷阪ｒ SetJudgeImage 縺ｫ謌ｻ縺励∪縺励◆
	// 蜷榊燕縺ｯImage縺ｧ縺吶′縲∽ｸｭ霄ｫ縺ｯ繝・く繧ｹ繝医ｒ譖ｴ譁ｰ縺励∪縺・
	void IngameCanvas::SetJudgeImage(JudgeResult result)
	{
		// 笘・ullptr繝√ぉ繝・け (縺薙ｌ縺後↑縺・→繧ｯ繝ｩ繝・す繝･縺励∪縺・
		if (!judgeResultText) return;

		// 判定表示位置にリセット (カウントダウンで中央に移動させていたため) -> 削除
		// judgeResultText->transform.SetPosition(Vector3(0, -100, 0));
		// judgeResultText->transform.SetScale(Vector3(4.5f, 1.15f, 0)); // Updateで毎フレーム制御するためここではセットしない

		switch (result) {
		case JudgeResult::Perfect:
			judgeResultText->SetText(L"PERFECT");
			judgeResultText->material.SetColor({ 1.0f, 0.8f, 0.0f, 1.0f }); // 驥・
			break;

		case JudgeResult::Great:
			judgeResultText->SetText(L"GREAT");
			judgeResultText->material.SetColor({ 1.0f, 0.4f, 0.7f, 1.0f }); // 繝斐Φ繧ｯ
			break;

		case JudgeResult::Good:
			judgeResultText->SetText(L"GOOD");
			judgeResultText->material.SetColor({ 0.2f, 1.0f, 0.2f, 1.0f }); // 邱・
			break;

		case JudgeResult::Miss:
			judgeResultText->SetText(L"MISS");
			judgeResultText->material.SetColor({ 0.6f, 0.6f, 0.6f, 1.0f }); // 轣ｰ
			break;

		case JudgeResult::None:
		default:
			// None縺ｮ譎ゅ・菴輔ｂ陦ｨ遉ｺ縺励↑縺・ｭ峨・蜃ｦ逅・
			judgeResultText->SetText(L"");
			break;
		}
	}

	void IngameCanvas::UpdateCountdownDisplay(float time, bool isStart)
	{
		if (!countdownText) return;

		// フラグ更新
		isCountdownActive = (time > 0.0f) || isStart;

        // Animation Parameters
        float animT = 0.0f; 
        
		if (isStart) {
            // START!! Animation
            // Assuming startDisplayTimer logic in IngameScene keeps calling this with isStart=true 
            // We don't have the timer value passed here for START logic in current signature...
            // But we can just behave static or pulse if called repeatedly.
            // Wait, IngameScene calls UpdateCountdownDisplay(0.0f, true) ONCE, then relies on ... wait.
            // IngameScene doesn't call this every frame for START?
            // Checking IngameScene.cpp:
            // It calls UpdateCountdownDisplay(0.0f, true) when countdown <= 0.
            // Then loop continues in Playing state.
            // It calls UpdateCountdownDisplay(-1.0f, false) after 1.0s.
            // So for START, we only get ONE call? If so, we can't animate it easily without Update() support.
            
            // However, countdown numbers (3, 2, 1) are called every frame in Countdown state.
            // So 3, 2, 1 CAN be animated.
            
			if (!isCountdownStartShown) {
				countdownText->SetText(L"START!!");
				countdownText->material.SetColor({ 1, 0, 0, 1 }); // Red
				countdownText->transform.SetScale(Vector3(12, 4, 0)); 
				isCountdownStartShown = true;
                
                // For "START", let's just set a big scale. 
                // Real animation for START requires per-frame update. 
                // But user asked for "Countdown" animation (3,2,1).
			}
		}
		else if (time > 0.0f) {
			isCountdownStartShown = false;
			int count = (int)ceil(time);
            
            // Fractional part for animation: 1.0 -> 0.0
            float frac = time - floor(time);
            if (frac == 0.0f && time > 0.0f) frac = 1.0f; // Handle exact integers

            // Animation: Pop in
            // Scale: Large -> Normal
            // Alpha: 0 -> 1 (Fade In) or 1 -> 0 (Fade Out)?
            // "Common countdown": 3 (Bam!) -> fade/shrink -> 2 (Bam!)
            
            // t goes 1.0 -> 0.0 as time decreases.
            // effectively: starts at integer (t=0 or 1?), decreases.
            // Let's us animate from frac=1.0 (start of second) down to 0.0 (end of second).
            // Actually time decreases: 3.0 -> 2.9 -> ...
            // So frac is 0.9 -> 0.0.
            // Wait, ceil(2.9) = 3. 
            // We want animation to start at 2.999...
            
            // Let's use 1.0 - frac as "progress".
            // progress: 0.0 (start of beat) -> 1.0 (end of beat)
            // But time isn't perfectly synced to beats, checking countdownTimer.
            // countdownTimer starts at 3.0 and goes down.
            
            // Let's calculate 't' as (1.0 - frac).
            // At 2.99: frac=0.99, t=0.01 (Just started)
            // At 2.50: frac=0.50, t=0.50
            // At 2.01: frac=0.01, t=0.99 (About to switch)
            
            // Desired effect: Scale 2.0 -> 1.0, Alpha 0.5 -> 1.0 -> 0.0?
            // Usually: Boom (Scale 1.5, Alpha 1) -> Shrink to 1.0, Alpha remains 1?
            // Or Fade out?
            
            float t = 1.0f - frac; // 0.0 (Start of number) -> 1.0 (End)
            
            // Easing: Elastic Out or similar? Simple exponential decay is fine.
            float scale = 1.0f + 1.0f * (1.0f - t) * (1.0f - t); // 2.0 -> 1.0
            float alpha = 1.0f;
            // Fade out at the very end?
            if (t > 0.8f) {
                alpha = 1.0f - (t - 0.8f) / 0.2f; 
            }
            
            // Clamp
            float baseScaleW = 10.0f;
            float baseScaleH = 5.0f;
			
			if (count != lastCountdownVal) {
				wchar_t buf[16];
				swprintf_s(buf, L"%d", count);
				countdownText->SetText(buf);
				countdownText->material.SetColor({ 1, 1, 0, 1 }); // Yellow
				lastCountdownVal = count;
			}
            
            countdownText->transform.SetScale(Vector3(baseScaleW * scale, baseScaleH * scale, 0));
            countdownText->material.SetColor({ 1, 1, 0, alpha });
		}
		else {
			if (lastCountdownVal != 0 || isCountdownStartShown) {
				countdownText->SetText(L"");
				lastCountdownVal = 0;
			}
			isCountdownStartShown = false;
		}
	}

	void IngameCanvas::UpdateJacketImage() {
		auto actor = actorRef.Target();
		if (!actor) return;

		// DI経由で渡されたSongInfoを使用
		if (!songInfoPtr) return;

		bool loadSuccess = false;
		if (!songInfoPtr->jacketPath.empty()) {
			// 日本語パス対応: UTF-8 → Shift-JIS
			std::string sjisPath = sf::util::Utf8ToShiftJis(songInfoPtr->jacketPath);
			loadSuccess = textureJacket.LoadTextureFromFile(sjisPath);
		}

		if (Jacket) {
			Jacket->material.texture = loadSuccess ? &textureJacket : &textureDefaultJacket;
		}

        // Update Song Info Text
        if (titleText) {
            std::wstring wTitle = sf::util::Utf8ToWstring(songInfoPtr->title);
            titleText->SetText(wTitle.c_str());
        }

        if (bpmText) {
             std::wstring wBpm = L"BPM: " + sf::util::Utf8ToWstring(songInfoPtr->bpm);
             bpmText->SetText(wBpm.c_str());
        }

        if (difficultyText) {
            // Difficulty ID -> String or use Level?
            // Usually display LEVEL: X
            wchar_t buf[32];
            swprintf_s(buf, L"Lv.%d", songInfoPtr->level);
            difficultyText->SetText(buf);
            
            // Optional: Color by Difficulty? 
            // 0:BASIC(Green), 1:ADVANCED(Orange), 2:EXPERT(Red), 3:MASTER(Purple)
            D2D1_COLOR_F difColor = D2D1::ColorF(D2D1::ColorF::White);
            switch(songInfoPtr->difficulty) {
                case 0: difColor = D2D1::ColorF(D2D1::ColorF::LimeGreen); break;
                case 1: difColor = D2D1::ColorF(D2D1::ColorF::Orange); break;
                case 2: difColor = D2D1::ColorF(D2D1::ColorF::Red); break;
                case 3: difColor = D2D1::ColorF(0.8f, 0.0f, 1.0f); break; // Purple
            }
            difficultyText->material.SetColor({difColor.r, difColor.g, difColor.b, 1.0f});
        }
	}

	void IngameCanvas::DestroyEffect(sf::ui::Image* effect) {
		if (!effect) return;
		uis.remove(effect);
		delete effect;
	}



    // ...

	void IngameCanvas::ShowFastSlow(int type) {
		if (!fastSlowText) return;

		if (type == 1) { // FAST
			fastSlowText->SetText(L"FAST");
			fastSlowText->material.SetColor({ 0.0f, 0.5f, 1.0f, 1.0f }); // Blue
			fastSlowScaleTimer = 0.10f; // 脈動開始
		}
		else if (type == 2) { // SLOW
			fastSlowText->SetText(L"SLOW");
			fastSlowText->material.SetColor({ 1.0f, 0.2f, 0.2f, 1.0f }); // Red
			fastSlowScaleTimer = 0.10f; // 脈動開始
		}
		else {
			fastSlowText->SetText(L"");
		}
	}



    void IngameCanvas::Draw() {
        // Draw standard UI (Texts, Images)
        sf::ui::Canvas::Draw();
        
        // Draw Custom Gauge
        DrawScoreGauge();
    }

    void IngameCanvas::DrawScoreGauge() {
        if (!noteManager) return;
        int maxCombo = noteManager->GetMaxTotalCombo();
        if (maxCombo <= 0) maxCombo = 1;

        int perfect = GetCurrentSession().GetCount(JudgeResult::Perfect);
        int great = GetCurrentSession().GetCount(JudgeResult::Great);
        int good = GetCurrentSession().GetCount(JudgeResult::Good);
        int miss = GetCurrentSession().GetCount(JudgeResult::Miss);

        // Simple Score based progress
        double currentSum = perfect * 1.0 + great * 0.8 + good * 0.5;
        double maxPossible = (double)maxCombo;
        float ratio = (float)(currentSum / maxPossible);
        if (ratio > 1.0f) ratio = 1.0f;
        if (ratio < 0.0f) ratio = 0.0f;

        // Visual Parameters
        float barWidth = 600.0f; // User requested 600
        float barHeight = 50.0f; // User requested 50
        float baseY = 350.0f; // Lowered from 400.0f to avoid overlap with Score Text
        
        // Calculate Rank Color (Synced with ResultCanvas)
        int score = (int)((currentSum / maxPossible) * 1000000.0);
        if (score > 1000000) score = 1000000;

        DirectX::XMFLOAT4 rankColor = { 0.8f, 1.0f, 0.8f, 1.0f }; // C (Green)
        
        if (score >= 1000000) { rankColor = { 1.0f, 1.0f, 0.9f, 1.0f }; } // SSS (Platinum/Gold)
        else if (score >= 900000) { rankColor = { 1.0f, 1.0f, 0.6f, 1.0f }; } // SS (Gold)
        else if (score >= 800000) { rankColor = { 1.0f, 0.95f, 0.4f, 1.0f }; } // S (Gold Std)
        else if (score >= 600000) { rankColor = { 1.0f, 0.8f, 0.8f, 1.0f }; } // A (Red)
        else if (score >= 400000) { rankColor = { 0.8f, 0.9f, 1.0f, 1.0f }; } // B (Blue)
        // Else C (Green)

        auto* dx11 = sf::dx::DirectX11::Instance();
        auto device = dx11->GetMainDevice().GetDevice();
        auto context = dx11->GetMainDevice().GetContext();

        // Common Setup
        dx11->gsScoreGauge.SetGPU(dx11->GetMainDevice());
        dx11->psScoreGauge.SetGPU(dx11->GetMainDevice()); 
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

        // Calculate Alignment Position
        // Score Text is at -750 (Center). Shifting gauge left to align with text start.
        float leftEdgeX = -900.0f;

        // --- Layer 1: Background Container ---
        {
            float bgWidth = barWidth;
            // Geometry is [-1, 1] (Size 2). So Scale should be width/2.
            float scaleX = bgWidth * 0.5f;
            float scaleY = barHeight * 0.5f;
            
            // Center is LeftEdge + HalfWidth
            float bgCenterX = leftEdgeX + scaleX;
            
            DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scaleX, scaleY, 1.0f);
            DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(bgCenterX, baseY, 0.0f);
            
            WorldMatrixBuffer wb;
            wb.mtx = DirectX::XMMatrixTranspose(S * T);
            wb.rot = DirectX::XMMatrixIdentity();
            dx11->wBuffer.SetGPU(wb, dx11->GetMainDevice());

            // Material
            mtl material;
            material.diffuseColor = { 0.2f, 0.2f, 0.2f, 0.8f }; 
            float aspect = bgWidth / barHeight;
            material.emissionColor = { aspect, 0, 0, 0 }; 
            dx11->mtlBuffer.SetGPU(material, dx11->GetMainDevice());

            dx11->vsNone.SetGPU(dx11->GetMainDevice()); 
            context->Draw(1, 0); 
        }

        // --- Layer 2: Foreground Bar ---
        if (ratio > 0.001f) 
        {
            float currentWidth = barWidth * ratio;
            
            float scaleX = currentWidth * 0.5f;
            float scaleY = barHeight * 0.5f;
            float centerX = leftEdgeX + scaleX;

            DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scaleX, scaleY, 1.0f);
            DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(centerX, baseY, 0.0f);
            
            WorldMatrixBuffer wb;
            wb.mtx = DirectX::XMMatrixTranspose(S * T);
            wb.rot = DirectX::XMMatrixIdentity();
            dx11->wBuffer.SetGPU(wb, dx11->GetMainDevice());

            // Material
            mtl material;
            material.diffuseColor = rankColor;
            float aspect = currentWidth / barHeight;
            if (aspect < 1.0f) aspect = 1.0f; // Fix visual artifact if extremely small
            material.emissionColor = { aspect, 0, 0, 0 }; 
            dx11->mtlBuffer.SetGPU(material, dx11->GetMainDevice());

            context->Draw(1, 0); 
        }

        // --- Rank Markers (Lines & Text Update) ---
        // Thresholds: B(400k), A(600k), S(800k)
        float thresholds[] = { 0.4f, 0.6f, 0.8f };
        
        // Use simple simple texture/color PS for lines (Not rounded)
        dx11->ps2d.SetGPU(dx11->GetMainDevice()); 

        for (int i = 0; i < 3; i++) {
            float th = thresholds[i];
            
            // Calculate X position
            float markX = leftEdgeX + barWidth * th;
            
            // Draw Line (White, Thin)
            float lineHeight = barHeight + 10.0f;
            
            DirectX::XMMATRIX S = DirectX::XMMatrixScaling(1.5f, lineHeight * 0.5f, 1.0f); // Width 3px total
            DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(markX, baseY, 0.0f);
            
            WorldMatrixBuffer wb;
            wb.mtx = DirectX::XMMatrixTranspose(S * T);
            wb.rot = DirectX::XMMatrixIdentity();
            dx11->wBuffer.SetGPU(wb, dx11->GetMainDevice());
            
            mtl material;
            material.diffuseColor = { 1.0f, 1.0f, 1.0f, 0.8f }; 
            dx11->mtlBuffer.SetGPU(material, dx11->GetMainDevice());
            
            dx11->vsNone.SetGPU(dx11->GetMainDevice()); 
            context->Draw(1, 0);
            
            // Update Label Position
            if (rankLabels[i]) {
                float labelY = baseY + 45.0f; // Lowered from 55.0f
                rankLabels[i]->transform.SetPosition(Vector3(markX, labelY, 0));
            }
        }

        // Restore Standard
        dx11->ps2d.SetGPU(dx11->GetMainDevice());
    }

    // --- Background Particles Removed (Moved to IngameScene 3D) ---
} // namespace app::test
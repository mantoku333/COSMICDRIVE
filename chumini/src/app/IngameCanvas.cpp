#include "IngameCanvas.h"
#include <sstream>
#include <iomanip>
#include "sf/Time.h"
#include "NoteManager.h"
#include "GameSession.h"
#include <algorithm>
#include "DirectX11.h"
#include "StringUtils.h"  // 譁・ｭ励さ繝ｼ繝牙､画鋤繝ｦ繝ｼ繝・ぅ繝ｪ繝・ぅ

namespace app::test {

    namespace Colors {
        constexpr DirectX::XMFLOAT4 RankSSS = { 1.0f, 1.0f, 0.9f, 1.0f }; // Platinum/Gold
        constexpr DirectX::XMFLOAT4 RankSS = { 1.0f, 1.0f, 0.6f, 1.0f }; // Gold
        constexpr DirectX::XMFLOAT4 RankS = { 1.0f, 0.95f, 0.4f, 1.0f }; // Gold Std
        constexpr DirectX::XMFLOAT4 RankA = { 1.0f, 0.8f, 0.8f, 1.0f }; // Red
        constexpr DirectX::XMFLOAT4 RankB = { 0.8f, 0.9f, 1.0f, 1.0f }; // Blue
        constexpr DirectX::XMFLOAT4 RankC = { 0.8f, 1.0f, 0.8f, 1.0f }; // Green
    }

	void IngameCanvas::Begin()
	{
		sf::ui::Canvas::Begin();

		auto* dx11 = sf::dx::DirectX11::Instance();
		auto context = dx11->GetMainDevice().GetDevice();

		// =========================================================
		// 郢昴・縺醍ｹｧ・ｹ郢昶・ﾎ暮坡・ｭ邵ｺ・ｿ髴趣ｽｼ邵ｺ・ｿ
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
		// UI騾墓ｻ薙・
		// =========================================================
		Jacket = AddUI<sf::ui::Image>();
		Jacket->transform.SetPosition(Vector3(810, 400, 0));
		Jacket->transform.SetScale(Vector3(3, 3, 0));
		Jacket->material.texture = &textureJacket;




		// ---------------------------------------------------------
		// 郢昴・縺冗ｹｧ・ｹ郢昴・I騾墓ｻ薙・
		// ---------------------------------------------------------


		// 2. 郢ｧ・ｳ郢晢ｽｳ郢昴・
		comboText = AddUI<sf::ui::TextImage>();
		comboText->transform.SetPosition(Vector3(650, -100, 0)); // Raised by 50 (-150 -> -100)
		comboText->transform.SetScale(Vector3(4.5f, 1.8f, 0));
		if (!comboText->Create(context, L"", L"Assets/Fonts/\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8.ttf", 100.0f, D2D1::ColorF(D2D1::ColorF::Cyan), 512, 256)) {
			sf::debug::Debug::LogError("IngameCanvas: Failed to create comboText");
		}

		// 3. 蛻､螳壼・險ｳ (蛟句挨陦ｨ遉ｺ)
		// 蟾ｦ遶ｯ (-960 ~ )縲ょｰ代＠蜿ｳ縺ｫ蟇・○縺ｦ -700 莉倩ｿ・
		// Y蠎ｧ讓吶・隱ｿ謨ｴ

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


		// 4. 蛻､螳夂ｵ先棡陦ｨ遉ｺ
		judgeResultText = AddUI<sf::ui::TextImage>();
		judgeResultText->transform.SetPosition(Vector3(0, -200, 0)); // Raised by 50 (-250 -> -200)
		judgeResultText->transform.SetScale(Vector3(4.5f, 1.15f, 0));
		if (!judgeResultText->Create(
			context,
			L"", // 譛蛻昴・遨ｺ
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

		// 5. 繧ｫ繧ｦ繝ｳ繝医ム繧ｦ繝ｳ逕ｨ
		countdownText = AddUI<sf::ui::TextImage>();
		countdownText->transform.SetPosition(Vector3(0, 100, 0));
		countdownText->transform.SetScale(Vector3(10, 5, 0)); // 蛻晄悄繧ｵ繧､繧ｺ
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
		// 蛻晄悄蛹・
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
		
		// 繧ｫ繧ｦ繝ｳ繝医ム繧ｦ繝ｳ迥ｶ諷句・譛溷喧
		lastCountdownVal = -1;
		isCountdownStartShown = false;

		updateCommand.Bind(std::bind(&IngameCanvas::Update, this, std::placeholders::_1));
	}

	void IngameCanvas::Update(const sf::command::ICommand&)
	{
		countUpTimer += sf::Time::DeltaTime();



		int combo = GetCurrentSession().GetCount(JudgeResult::Perfect) + GetCurrentSession().GetCount(JudgeResult::Great) + GetCurrentSession().GetCount(JudgeResult::Good); // 邁｡譏楢ｨ育ｮ・or GetCurrentSession().GetCombo() if available

		// ... (繧ｳ繝ｳ繝懷宛蠕｡縺ｯ縺ゅ→縺ｧ)
		// GetCurrentSession().GetCombo() 縺後≠繧後・縺昴ｌ繧剃ｽｿ縺・
		combo = GetCurrentSession().GetCombo();

		// 繧ｳ繝ｳ繝懷､牙虚譎ゅ・蛻晄悄蛹・閼亥虚蛻､螳・
		if (lastCombo == -1) {
			lastCombo = combo;
		}
		else if (combo != lastCombo) {
			if (combo > lastCombo) {
				// 繧ｳ繝ｳ繝懊′蠅励∴縺溘→縺阪□縺題ц蜍輔＆縺帙ｋ縲ゅｈ繧贋ｿ頑撫縺ｫ・・.1遘抵ｼ・
				comboScaleTimer = 0.10f; 
			}
			lastCombo = combo;
		}

		// 繧ｳ繝ｳ繝懆ц蜍輔い繝九Γ繝ｼ繧ｷ繝ｧ繝ｳ
		float scaleRate = 1.0f;
		if (comboScaleTimer > 0.0f) {
			comboScaleTimer -= sf::Time::DeltaTime();
			if (comboScaleTimer < 0.0f) comboScaleTimer = 0.0f;
			
			// 蜊倡ｴ斐↑邱壼ｽ｢陬憺俣縺ｧ諡｡螟ｧ邇・ｒ豎ｺ繧√ｋ・亥ｿ・ｦ√↑繧右asing繧貞・繧後ｋ・・
			// 譛螟ｧ1.6蛟阪￥繧峨＞縺ｾ縺ｧ螟ｧ縺阪￥縺吶ｋ
			float t = comboScaleTimer / 0.10f; // 1.0 -> 0.0
			scaleRate = 1.0f + t * 0.6f;
		}

		if (comboText) {
			// 螳壽焚・壼・譛溘せ繧ｱ繝ｼ繝ｫ (4.5, 1.8, 0)
			comboText->transform.SetScale(Vector3(4.5f * scaleRate, 1.8f * scaleRate, 0.0f));
		}

		UpdateComboDisplay(combo);
		UpdateJudgeCountDisplay();

		// Update縺ｧ縺ｮ蛻､螳夂ｵ先棡譖ｴ譁ｰ
		JudgeResult currentJudge = GetCurrentSession().GetLastResult();
		if (currentJudge != lastJudgeResult) {
			SetJudgeImage(currentJudge);
			lastJudgeResult = currentJudge;
		}

		// 蛻､螳夊ц蜍輔Ο繧ｸ繝・け
		// 繧ｫ繧ｦ繝ｳ繝医ム繧ｦ繝ｳ荳ｭ縺ｯ驕ｩ逕ｨ縺励↑縺・
		if (!isCountdownActive) {
			int currentTotalJudges = GetCurrentSession().GetCount(JudgeResult::Perfect)
				+ GetCurrentSession().GetCount(JudgeResult::Great)
				+ GetCurrentSession().GetCount(JudgeResult::Good)
				+ GetCurrentSession().GetCount(JudgeResult::Miss);

			if (lastTotalJudges == -1) {
				lastTotalJudges = currentTotalJudges;
			}
			else if (currentTotalJudges > lastTotalJudges) {
				judgeScaleTimer = 0.10f; // 閼亥虚髢句ｧ・
				lastTotalJudges = currentTotalJudges;
			}

			float judgeScaleRate = 1.0f;
			if (judgeScaleTimer > 0.0f) {
				judgeScaleTimer -= sf::Time::DeltaTime();
				if (judgeScaleTimer < 0.0f) judgeScaleTimer = 0.0f;

				float t = judgeScaleTimer / 0.10f;
				judgeScaleRate = 1.0f + t * 0.5f; // Combox0.6縺ｫ霑代＞0.5蛟肴僑螟ｧ
			}

			if (judgeResultText) {
				// Base Scale (4.5, 1.15)
				judgeResultText->transform.SetScale(Vector3(4.5f * judgeScaleRate, 1.15f * judgeScaleRate, 0.0f));
			}
		}

		// ---------------------------------------------------------
		// FAST/SLOW 閼亥虚繝ｭ繧ｸ繝・け
		// ---------------------------------------------------------
		float fsScaleRate = 1.0f;
		if (fastSlowScaleTimer > 0.0f) {
			fastSlowScaleTimer -= sf::Time::DeltaTime();
			if (fastSlowScaleTimer < 0.0f) fastSlowScaleTimer = 0.0f;

			float t = fastSlowScaleTimer / 0.10f;
			fsScaleRate = 1.0f + t * 0.5f; // Combox0.6縺ｫ霑代＞0.5蛟肴僑螟ｧ
		}

		if (fastSlowText) {
			// Base Scale (1.5, 0.5)
			fastSlowText->transform.SetScale(Vector3(1.5f * fsScaleRate, 0.5f * fsScaleRate, 0.0f));
		}

        // Update Score Display with calculated values from Model (GameSession)
        auto& session = GetCurrentSession();
        UpdateScoreDisplay(session.CalculateScore(), session.GetRank());
	}

    void IngameCanvas::UpdateScoreDisplay(int score, GameSession::Rank rank) {
        if (!scoreText || !rankText) return;

        // Score
        wchar_t buf[32];
        swprintf_s(buf, L"%07d", score);
        scoreText->SetText(buf);

        // Rank
        std::wstring rankStr = L"C";
        DirectX::XMFLOAT4 rankColor = Colors::RankC;

        switch (rank) {
        case GameSession::Rank::SSS: rankStr = L"SSS"; rankColor = Colors::RankSSS; break;
        case GameSession::Rank::SS:  rankStr = L"SS";  rankColor = Colors::RankSS;  break;
        case GameSession::Rank::S:   rankStr = L"S";   rankColor = Colors::RankS;   break;
        case GameSession::Rank::A:   rankStr = L"A";   rankColor = Colors::RankA;   break;
        case GameSession::Rank::B:   rankStr = L"B";   rankColor = Colors::RankB;   break;
        default:                     rankStr = L"C";   rankColor = Colors::RankC;   break;
        }

        rankText->SetText(rankStr.c_str());
        rankText->material.SetColor(rankColor);
    }

	void IngameCanvas::SpawnHitEffect(float x, float y, float scale, float duration, const Color& color)
	{
		if (!effectManager.isNull()) {
			// 繧ｹ繝励Λ繧､繝育函謌舌・縺ｿ
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
	// 髯ｦ・ｨ驕会ｽｺ隴厄ｽｴ隴・ｽｰ鬮｢・｢隰ｨ・ｰ
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

	// 隨倥・譛ｪ隰ｨ・ｰ陷ｷ髦ｪ・・SetJudgeImage 邵ｺ・ｫ隰鯉ｽｻ邵ｺ蜉ｱ竏ｪ邵ｺ蜉ｱ笳・
	// 陷ｷ讎顔√邵ｺ・ｯImage邵ｺ・ｧ邵ｺ蜷ｶ窶ｲ邵ｲ竏ｽ・ｸ・ｭ髴・ｽｫ邵ｺ・ｯ郢昴・縺冗ｹｧ・ｹ郢晏現・定ｭ厄ｽｴ隴・ｽｰ邵ｺ蜉ｱ竏ｪ邵ｺ繝ｻ
	void IngameCanvas::SetJudgeImage(JudgeResult result)
	{
		// 隨倥・ullptr郢昶・縺臥ｹ昴・縺・(邵ｺ阮呻ｽ檎ｸｺ蠕娯・邵ｺ繝ｻ竊堤ｹｧ・ｯ郢晢ｽｩ郢昴・縺咏ｹ晢ｽ･邵ｺ蜉ｱ竏ｪ邵ｺ繝ｻ
		if (!judgeResultText) return;

		// 蛻､螳夊｡ｨ遉ｺ菴咲ｽｮ縺ｫ繝ｪ繧ｻ繝・ヨ (繧ｫ繧ｦ繝ｳ繝医ム繧ｦ繝ｳ縺ｧ荳ｭ螟ｮ縺ｫ遘ｻ蜍輔＆縺帙※縺・◆縺溘ａ) -> 蜑企勁
		// judgeResultText->transform.SetPosition(Vector3(0, -100, 0));
		// judgeResultText->transform.SetScale(Vector3(4.5f, 1.15f, 0)); // Update縺ｧ豈弱ヵ繝ｬ繝ｼ繝蛻ｶ蠕｡縺吶ｋ縺溘ａ縺薙％縺ｧ縺ｯ繧ｻ繝・ヨ縺励↑縺・

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
			// None邵ｺ・ｮ隴弱ｅ繝ｻ闖ｴ霈費ｽる勗・ｨ驕会ｽｺ邵ｺ蜉ｱ竊醍ｸｺ繝ｻ・ｭ蟲ｨ繝ｻ陷・ｽｦ騾・・
			judgeResultText->SetText(L"");
			break;
		}
	}

	void IngameCanvas::UpdateCountdownDisplay(float time, bool isStart)
	{
		if (!countdownText) return;

		// 繝輔Λ繧ｰ譖ｴ譁ｰ
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

		// DI邨檎罰縺ｧ貂｡縺輔ｌ縺欖ongInfo繧剃ｽｿ逕ｨ
		if (!songInfoPtr) return;

		bool loadSuccess = false;
		if (!songInfoPtr->jacketPath.empty()) {
			// 譌･譛ｬ隱槭ヱ繧ｹ蟇ｾ蠢・ UTF-8 竊・Shift-JIS
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
			fastSlowScaleTimer = 0.10f; // 閼亥虚髢句ｧ・
		}
		else if (type == 2) { // SLOW
			fastSlowText->SetText(L"SLOW");
			fastSlowText->material.SetColor({ 1.0f, 0.2f, 0.2f, 1.0f }); // Red
			fastSlowScaleTimer = 0.10f; // 閼亥虚髢句ｧ・
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
        // Use GameSession for data
        auto& session = GetCurrentSession();
        int score = session.CalculateScore();
        
        // Ratio: Score / 1000000
        float ratio = (float)score / 1000000.0f;
        if (ratio > 1.0f) ratio = 1.0f;
        if (ratio < 0.0f) ratio = 0.0f;

        // Visual Parameters
        float barWidth = 600.0f; // User requested 600
        float barHeight = 50.0f; // User requested 50
        float baseY = 350.0f; // Lowered from 400.0f to avoid overlap with Score Text
        
        // Calculate Rank Color
        DirectX::XMFLOAT4 rankColor = Colors::RankC;
        GameSession::Rank rank = session.GetRank();

        switch (rank) {
        case GameSession::Rank::SSS: rankColor = Colors::RankSSS; break;
        case GameSession::Rank::SS:  rankColor = Colors::RankSS;  break;
        case GameSession::Rank::S:   rankColor = Colors::RankS;   break;
        case GameSession::Rank::A:   rankColor = Colors::RankA;   break;
        case GameSession::Rank::B:   rankColor = Colors::RankB;   break;
        default:                     rankColor = Colors::RankC;   break;
        }

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
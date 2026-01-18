#include "IngameCanvas.h"
#include <sstream>
#include <iomanip>
#include "Time.h"
#include "NoteManager.h"
#include "IngameScene.h"
#include "JudgeStatsService.h"
#include <algorithm>
#include "DirectX11.h"

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
		comboText->transform.SetPosition(Vector3(650, 0, 0));
		comboText->transform.SetScale(Vector3(4.5f, 1.8f, 0));
		if (!comboText->Create(context, L"", L"851\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8", 100.0f, D2D1::ColorF(D2D1::ColorF::Cyan), 512, 256)) {
			sf::debug::Debug::LogError("IngameCanvas: Failed to create comboText");
		}

		// 3. 判定内訳 (個別表示)
		// 左端 (-960 ~ )。少し右に寄せて -700 付近
		// Y座標は調整

		// PERFECT
		perfectText = AddUI<sf::ui::TextImage>();
		perfectText->transform.SetPosition(Vector3(-700, 100, 0)); 
		perfectText->transform.SetScale(Vector3(3.5f, 1.5f, 0)); 
		if (!perfectText->Create(context, L"PERFECT: 0", L"851\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8", 50.0f, D2D1::ColorF(D2D1::ColorF::Yellow), 512, 128)) {
			sf::debug::Debug::LogError("IngameCanvas: Failed to create perfectText");
		}

		// GREAT
		greatText = AddUI<sf::ui::TextImage>();
		greatText->transform.SetPosition(Vector3(-700, 50, 0));
		greatText->transform.SetScale(Vector3(3.5f, 1.5f, 0));
		greatText->Create(context, L"GREAT: 0", L"851\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8", 50.0f, D2D1::ColorF(D2D1::ColorF::HotPink), 512, 128);

		// GOOD
		goodText = AddUI<sf::ui::TextImage>();
		goodText->transform.SetPosition(Vector3(-700, 0, 0));
		goodText->transform.SetScale(Vector3(3.5f, 1.5f, 0));
		goodText->Create(context, L"GOOD: 0", L"851\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8", 50.0f, D2D1::ColorF(D2D1::ColorF::LimeGreen), 512, 128);

		// MISS
		missText = AddUI<sf::ui::TextImage>();
		missText->transform.SetPosition(Vector3(-700, -50, 0));
		missText->transform.SetScale(Vector3(3.5f, 1.5f, 0));
		missText->Create(context, L"MISS: 0", L"851\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8", 50.0f, D2D1::ColorF(D2D1::ColorF::Gray), 512, 128);


		// 4. 判定結果表示
		judgeResultText = AddUI<sf::ui::TextImage>();
		judgeResultText->transform.SetPosition(Vector3(0, -100, 0));
		judgeResultText->transform.SetScale(Vector3(4.5f, 1.15f, 0));
		if (!judgeResultText->Create(
			context,
			L"", // 最初は空
			L"851\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8",
			120.0f,
			D2D1::ColorF(D2D1::ColorF::White),
			1024, 256
		)) {
			sf::debug::Debug::LogError("IngameCanvas: Failed to create judgeResultText");
		}

		// FAST/SLOW
		fastSlowText = AddUI<sf::ui::TextImage>();
		fastSlowText->transform.SetPosition(Vector3(0, -200, 0));
		fastSlowText->transform.SetScale(Vector3(1.5f, 0.5f, 0)); // 小さく変更 (3.0, 1.0 -> 1.5, 0.5)
		if (!fastSlowText->Create(context, L"", L"851\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8", 80.0f, D2D1::ColorF(D2D1::ColorF::White), 512, 128)) {
			sf::debug::Debug::LogError("IngameCanvas: Failed to create fastSlowText");
		}

		// 5. カウントダウン用
		countdownText = AddUI<sf::ui::TextImage>();
		countdownText->transform.SetPosition(Vector3(0, 100, 0));
		countdownText->transform.SetScale(Vector3(10, 5, 0)); // 初期サイズ
		countdownText->Create(
			context,
			L"",
			L"851\x30B4\x30C1\x30AB\x30AF\x30C3\x30C8",
			120.0f,
			D2D1::ColorF(D2D1::ColorF::Yellow),
			1024, 256
		);




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



		int combo = JudgeStatsService::GetCount(JudgeResult::Perfect) + JudgeStatsService::GetCount(JudgeResult::Great) + JudgeStatsService::GetCount(JudgeResult::Good); // 簡易計算 or JudgeStatsService::GetCombo() if available

		// ... (コンボ制御はあとで)
		// JudgeStatsService::GetCombo() があればそれを使う
		combo = JudgeStatsService::GetCombo();

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
		JudgeResult currentJudge = JudgeStatsService::GetLastResult();
		if (currentJudge != lastJudgeResult) {
			SetJudgeImage(currentJudge);
			lastJudgeResult = currentJudge;
		}

		// 判定脈動ロジック
		// カウントダウン中は適用しない
		if (!isCountdownActive) {
			int currentTotalJudges = JudgeStatsService::GetCount(JudgeResult::Perfect)
				+ JudgeStatsService::GetCount(JudgeResult::Great)
				+ JudgeStatsService::GetCount(JudgeResult::Good)
				+ JudgeStatsService::GetCount(JudgeResult::Miss);

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
	}

	void IngameCanvas::SpawnHitEffect(float x, float y, float scale, float duration, const Color& color)
	{
		if (!effectManager.isNull()) {
			// 笘・せ繝励Λ繧､繝亥・逕溘・縺ｿ
			effectManager->SpawnSprite(x, y, scale, duration, color);
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
		int p = JudgeStatsService::GetCount(JudgeResult::Perfect);
		int gr = JudgeStatsService::GetCount(JudgeResult::Great);
		int go = JudgeStatsService::GetCount(JudgeResult::Good);
		int m = JudgeStatsService::GetCount(JudgeResult::Miss);

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

		// カウントダウン中は中央に表示（別途UIにしたので位置制御不要）
		// judgeResultText->transform.SetPosition(Vector3(0, 100, 0));
		
		// ★重要: カウントダウン中はタイマー更新を止めるフラグを立てる（未実装ならここで強制的にタイマーテキスト更新をスキップする仕組みを入れる）
		// ここでは簡易的に、メンバ変数 isCountdownActive を追加して管理するか、
		// もしくは IngameScene 側で制御する。
		// 今回は IngameCanvas::Update でカウントダウン中判定を行うために isCountdownStartShown を流用するか、
		// countdownTimerの値を見る。しかし countdownTimer はここだけのローカル引数。
		
		// 解決策: UpdateTimerDisplay の呼び出し元 (IngameCanvas::Update) を制御したいが、
		// ここではとりあえずテキスト更新を行う。

		if (isStart) {
			if (!isCountdownStartShown) {
				countdownText->SetText(L"START!!");
				countdownText->material.SetColor({ 1, 0, 0, 1 }); // 赤
				countdownText->transform.SetScale(Vector3(12, 3, 0)); // 少し大きく
				isCountdownStartShown = true;
			}
		}
		else if (time > 0.0f) {
			isCountdownStartShown = false;
			// ceilして 3, 2, 1 を表示
			int count = (int)ceil(time);
			
			if (count != lastCountdownVal) {
				wchar_t buf[16];
				swprintf_s(buf, L"%d", count);
				countdownText->SetText(buf);
				countdownText->material.SetColor({ 1, 1, 0, 1 }); // 黄
				countdownText->transform.SetScale(Vector3(10, 5, 0)); // 元のサイズ
				lastCountdownVal = count;
			}
		}
		else {
			if (lastCountdownVal != 0 || isCountdownStartShown) {
				countdownText->SetText(L"");
				lastCountdownVal = 0;
			}
			isCountdownStartShown = false;
		}
		
		// フラグ更新
		// 時間が0以上ならカウントダウン中、START表示中も含むが、START表示後はPlayingになるので呼ばれなくなるはず
		// 厳密には time > 0 || isStart
		isCountdownActive = (time > 0.0f) || isStart;
	}

	void IngameCanvas::UpdateJacketImage() {
		auto actor = actorRef.Target();
		if (!actor) return;

		auto& scene = actor->GetScene();
                auto* ingameScene = dynamic_cast<IngameScene*>(&scene);
                if (!ingameScene) return;

                const SongInfo& selectedSong = ingameScene->GetSelectedSong();

		bool loadSuccess = false;
		if (!selectedSong.jacketPath.empty()) {
			loadSuccess = textureJacket.LoadTextureFromFile(selectedSong.jacketPath);
		}

		if (Jacket) {
			Jacket->material.texture = loadSuccess ? &textureJacket : &textureDefaultJacket;
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

} // namespace app::test

#include "TestCanvas.h"
#include <sstream>
#include <iomanip>
#include "Time.h"
#include "NoteManager.h"
#include "TestScene.h"
#include "JudgeStatsService.h"
#include <algorithm> // std::min, std::max

namespace app::test {

	void TestCanvas::Begin()
	{
		// 基底クラスのBeginを必ず呼び出す
		sf::ui::Canvas::Begin();

		// =========================================================
		// テクスチャの読み込み
		// =========================================================
		textureJacket.LoadTextureFromFile("Assets\\Texture\\GOODTEK.png");
		textureDefaultJacket.LoadTextureFromFile("Assets\\Texture\\DefaultJacket.png");
		textureNone.LoadTextureFromFile("Assets\\Texture\\none.png");
		textureCombo.LoadTextureFromFile("Assets\\Texture\\combo.png");
		texturePanel.LoadTextureFromFile("Assets\\Texture\\SelectBack.png");

		texturePerfect.LoadTextureFromFile("Assets\\Texture\\perfect.png");
		textureGreat.LoadTextureFromFile("Assets\\Texture\\great.png");
		textureGood.LoadTextureFromFile("Assets\\Texture\\good.png");
		textureMiss.LoadTextureFromFile("Assets\\Texture\\miss.png");

		textureNumber.LoadTextureFromFile("Assets/Texture/numbers.png");

		// エフェクト用テクスチャ (Canvasが所有権を持つ)
		if (!textureHitEffect.LoadTextureFromFile("Assets\\Texture\\Effect-Tap.png")) {
			sf::debug::Debug::LogError("TestCanvas: Failed to load Effect-Tap.png");
		}

		// =========================================================
		// UIの生成と配置
		// =========================================================

		// ジャケット画像
		Jacket = AddUI<sf::ui::Image>();
		Jacket->transform.SetPosition(Vector3(810, 400, 0));
		Jacket->transform.SetScale(Vector3(3, 3, 0));
		Jacket->material.texture = &textureJacket;

		// 判定画像
		judgeImage = AddUI<sf::ui::Image>();
		judgeImage->transform.SetPosition(Vector3(810, 100, 0));
		judgeImage->transform.SetScale(Vector3(3, 1, 0));
		judgeImage->material.texture = &textureNone;

		// 判定パネル背景
		judgePanel = AddUI<sf::ui::Image>();
		judgePanel->transform.SetPosition(Vector3(0, 500, 0));
		judgePanel->transform.SetScale(Vector3(5.0f, 1.0f, 0));
		judgePanel->material.texture = &texturePanel;
		judgePanel->material.SetColor({ 1.0f, 1.0f, 1.0f, 0.9f });

		// =========================================================
		// 各種表示の初期化
		// =========================================================
		InitializeTimerDisplay();
		InitializeComboDisplay();
		InitializeJudgeCountDisplay();

		if (auto actor = actorRef.Target()) {

			// TestSceneでくっつけた EffectManager を探す
			if (auto manager = actor->GetComponent<EffectManager>()) {

				// メンバ変数に保存
				effectManager = manager;

				// 初期化 (Canvasとテクスチャを渡す)
				effectManager->Initialize(this, &textureHitEffect);
			}
		}

		// 同じActorのNoteManagerを取得
		if (auto actor = actorRef.Target()) {
			noteManager = actor->GetComponent<NoteManager>();
		}

		UpdateJacketImage();

		updateCommand.Bind(std::bind(&TestCanvas::Update, this, std::placeholders::_1));
	}

	// =========================================================
	// Update
	// =========================================================
	void TestCanvas::Update(const sf::command::ICommand&)
	{
		countUpTimer += sf::Time::DeltaTime();

		// タイマー表示更新
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(2) << countUpTimer;
		UpdateTimerDisplay(oss.str());

		// 判定情報の取得と表示更新
		int combo = JudgeStatsService::GetCombo();
		JudgeResult last = JudgeStatsService::GetLastResult();

		UpdateComboDisplay(combo);
		SetJudgeImage(last);
		UpdateJudgeCountDisplay();

	}

	// =========================================================
	// エフェクト生成 (NoteManagerから呼ばれる)
	// =========================================================
	void TestCanvas::SpawnHitEffect(float x, float y, float scale, float duration, const Color& color)
	{
		if (!effectManager.isNull()) {
			// そのままアロー演算子(->)でアクセス
			effectManager->Spawn(x, y, scale, duration, color);
		}
	}

	// =========================================================
	// 以下、省略されていたUI実装関数群
	// =========================================================

	void TestCanvas::InitializeTimerDisplay()
	{
		for (int i = 0; i < MAX_TIMER_DIGITS; ++i) {
			auto img = AddUI<sf::ui::Image>();
			img->transform.SetPosition(Vector3(800 + i * (digitWidth - 60), -100, 0));
			img->transform.SetScale(Vector3(0.5f, 0.5f, 0));
			img->material.texture = &textureNumber;
			img->SetVisible(false);
			timerDigits.push_back(img);
		}
	}

	void TestCanvas::InitializeComboDisplay()
	{
		for (int i = 0; i < MAX_COMBO_DIGITS; ++i) {
			auto img = AddUI<sf::ui::Image>();
			img->transform.SetPosition(Vector3(800 + i * (digitWidth - 60), 0, 0));
			img->transform.SetScale(Vector3(0.7f, 0.7f, 0));
			img->material.texture = &textureNumber;
			img->SetVisible(false);
			comboDigits.push_back(img);
		}
	}

	void TestCanvas::InitializeJudgeCountDisplay()
	{
		const float startX = -255;
		const float baseY = 500;
		const float gapX = 100;

		auto createDigitSet = [&](float x) {
			std::vector<sf::ui::Image*> digits;
			for (int i = 0; i < 4; ++i) {
				auto img = AddUI<sf::ui::Image>();
				img->transform.SetPosition(Vector3(x + i * (digitWidth - 60), baseY, 0));
				img->transform.SetScale(Vector3(0.3f, 0.3f, 0));
				img->material.texture = &textureNumber;
				img->SetVisible(false);
				digits.push_back(img);
			}
			return digits;
			};

		judgeCountDigitsPerfect = createDigitSet(startX + gapX * 0);
		judgeCountDigitsGreat = createDigitSet(startX + gapX * 1);
		judgeCountDigitsGood = createDigitSet(startX + gapX * 2);
		judgeCountDigitsMiss = createDigitSet(startX + gapX * 3);
	}

	void TestCanvas::UpdateTimerDisplay(const std::string& str)
	{
		for (auto* img : timerDigits) img->SetVisible(false);

		size_t displayCount = std::min(str.size(), static_cast<size_t>(MAX_TIMER_DIGITS));

		for (size_t i = 0; i < displayCount; ++i) {
			char ch = str[i];
			int digit = -1;
			bool isDot = false;

			if (ch >= '0' && ch <= '9') digit = ch - '0';
			else if (ch == '.') isDot = true;
			else continue;

			float u0, u1, v0 = 0.0f, v1 = digitHeight / sheetHeight;
			if (!isDot) {
				u0 = (digit * digitWidth) / sheetWidth;
				u1 = ((digit + 1) * digitWidth) / sheetWidth;
			}
			else {
				int dotIndex = 10;
				u0 = (dotIndex * digitWidth) / sheetWidth;
				u1 = ((dotIndex + 1) * digitWidth) / sheetWidth;
			}

			timerDigits[i]->SetUV(u0, v0, u1, v1);
			timerDigits[i]->SetVisible(true);
		}
	}

	void TestCanvas::UpdateComboDisplay(int combo)
	{
		for (auto* img : comboDigits) img->SetVisible(false);
		if (combo < 0) return;
		if (combo > 9999) combo = 9999;

		std::string comboStr = std::to_string(combo);
		int startIndex = MAX_COMBO_DIGITS - (int)comboStr.size();

		for (size_t i = 0; i < comboStr.size(); ++i) {
			int digit = comboStr[i] - '0';
			float u0 = (digit * digitWidth) / sheetWidth;
			float u1 = ((digit + 1) * digitWidth) / sheetWidth;
			float v0 = 0.0f;
			float v1 = digitHeight / sheetHeight;

			int displayIndex = startIndex + (int)i;
			if (displayIndex >= 0 && displayIndex < MAX_COMBO_DIGITS) {
				comboDigits[displayIndex]->SetUV(u0, v0, u1, v1);
				comboDigits[displayIndex]->SetVisible(true);
			}
		}
	}

	void TestCanvas::UpdateJudgeCountDisplay()
	{
		struct Entry { JudgeResult type; std::vector<sf::ui::Image*>& digits; };
		std::vector<Entry> entries = {
			{ JudgeResult::Perfect, judgeCountDigitsPerfect },
			{ JudgeResult::Great,   judgeCountDigitsGreat },
			{ JudgeResult::Good,    judgeCountDigitsGood },
			{ JudgeResult::Miss,    judgeCountDigitsMiss },
		};

		for (auto& e : entries)
		{
			int value = JudgeStatsService::GetCount(e.type);
			std::string str = std::to_string(value);
			int len = (int)str.size();
			int maxLen = (int)e.digits.size();
			for (int i = 0; i < maxLen; ++i)
				e.digits[i]->SetVisible(false);

			int start = maxLen - len;
			for (int i = 0; i < len; ++i)
			{
				int digit = str[i] - '0';
				float u0 = (digit * digitWidth) / sheetWidth;
				float u1 = ((digit + 1) * digitWidth) / sheetWidth;
				float v0 = 0.0f, v1 = digitHeight / sheetHeight;

				int index = start + i;
				if (index >= 0 && index < maxLen) {
					e.digits[index]->SetUV(u0, v0, u1, v1);
					e.digits[index]->SetVisible(true);
				}
			}
		}
	}

	void TestCanvas::DrawNumber(int number, float x, float y, float digitWidth, float digitHeight, float sheetWidth, float sheetHeight, sf::Texture* numberTexture)
	{
		std::string str = std::to_string(number);
		for (size_t i = 0; i < str.size(); ++i) {
			int digit = str[i] - '0';
			float u0 = (digit * digitWidth) / sheetWidth;
			float u1 = ((digit + 1) * digitWidth) / sheetWidth;
			float v0 = 0.0f;
			float v1 = digitHeight / sheetHeight;

			auto img = AddUI<sf::ui::Image>();
			img->transform.SetPosition(Vector3(x + i * (digitWidth - 60), y, 0));
			img->transform.SetScale(Vector3(0.3f, 0.3f, 0));
			img->material.texture = numberTexture;
			img->SetUV(u0, v0, u1, v1);
		}
	}

	void TestCanvas::SetJudgeImage(JudgeResult result)
	{
		switch (result) {
		case JudgeResult::Perfect:
			judgeImage->material.texture = &texturePerfect;
			break;
		case JudgeResult::Great:
			judgeImage->material.texture = &textureGreat;
			break;
		case JudgeResult::Good:
			judgeImage->material.texture = &textureGood;
			break;
		case JudgeResult::Miss:
			judgeImage->material.texture = &textureMiss;
			break;
		default:
			judgeImage->material.texture = &textureMiss;
			break;
		}
	}

	void TestCanvas::UpdateJacketImage() {
		auto actor = actorRef.Target();
		if (!actor) return;

		auto& scene = actor->GetScene();
		auto* testScene = dynamic_cast<TestScene*>(&scene);
		if (!testScene) return;

		const SongInfo& selectedSong = testScene->GetSelectedSong();

		bool loadSuccess = false;
		if (!selectedSong.jacketPath.empty()) {
			loadSuccess = textureJacket.LoadTextureFromFile(selectedSong.jacketPath);
			std::ostringstream oss;
			oss << "Loading jacket: " << selectedSong.jacketPath << " - "
				<< (loadSuccess ? "Success" : "Failed");
			sf::debug::Debug::Log(oss.str());
		}

		if (Jacket) {
			Jacket->material.texture = loadSuccess ? &textureJacket : &textureDefaultJacket;
		}
	}


	void TestCanvas::DestroyEffect(sf::ui::Image* effect) {
		if (!effect) return;

		// 親クラスのリスト(uis)から削除
		uis.remove(effect);

		// メモリ解放
		delete effect;
	}

} // namespace app::test
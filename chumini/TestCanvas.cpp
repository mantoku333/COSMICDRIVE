#include "TestCanvas.h"
#include <sstream>
#include <iomanip>
#include "Time.h"
#include "NoteManager.h"
#include "TestScene.h"
#include "JudgeStatsService.h"
#include <algorithm>
#include "DirectX11.h"

namespace app::test {

	void TestCanvas::Begin()
	{
		sf::ui::Canvas::Begin();

		auto* dx11 = sf::dx::DirectX11::Instance();
		auto context = dx11->GetMainDevice().GetDevice();

		// =========================================================
		// テクスチャ読み込み
		// =========================================================
		textureJacket.LoadTextureFromFile("Assets\\Texture\\GOODTEK.png");
		textureDefaultJacket.LoadTextureFromFile("Assets\\Texture\\DefaultJacket.png");
		texturePanel.LoadTextureFromFile("Assets\\Texture\\SelectBack.png");

		if (!textureHitEffect.LoadTextureFromFile("Assets\\Texture\\Effect-Tap.png")) {
			sf::debug::Debug::LogError("TestCanvas: Failed to load Effect-Tap.png");
		}

		// =========================================================
		// UI生成
		// =========================================================
		Jacket = AddUI<sf::ui::Image>();
		Jacket->transform.SetPosition(Vector3(810, 400, 0));
		Jacket->transform.SetScale(Vector3(3, 3, 0));
		Jacket->material.texture = &textureJacket;

		judgePanel = AddUI<sf::ui::Image>();
		judgePanel->transform.SetPosition(Vector3(0, 500, 0));
		judgePanel->transform.SetScale(Vector3(5.0f, 1.0f, 0));
		judgePanel->material.texture = &texturePanel;
		judgePanel->material.SetColor({ 1.0f, 1.0f, 1.0f, 0.9f });

		// ---------------------------------------------------------
		// テキストUI生成
		// ---------------------------------------------------------
		// 1. タイマー
		timerText = AddUI<sf::ui::TextImage>();
		timerText->transform.SetPosition(Vector3(800, -100, 0));
		timerText->transform.SetScale(Vector3(4, 1.5f, 0));
		timerText->Create(context, L"00.00", L"851ゴチカクット", 80.0f, D2D1::ColorF(D2D1::ColorF::White), 512, 128);

		// 2. コンボ
		comboText = AddUI<sf::ui::TextImage>();
		comboText->transform.SetPosition(Vector3(800, 0, 0));
		comboText->transform.SetScale(Vector3(5, 2, 0));
		comboText->Create(context, L"", L"851ゴチカクット", 100.0f, D2D1::ColorF(D2D1::ColorF::Cyan), 512, 256);

		// 3. 判定数内訳
		judgeStatsText = AddUI<sf::ui::TextImage>();
		judgeStatsText->transform.SetPosition(Vector3(-200, 500, 0));
		judgeStatsText->transform.SetScale(Vector3(5, 5, 0));
		judgeStatsText->Create(context, L"PERFECT: 0\n...", L"851ゴチカクット", 50.0f, D2D1::ColorF(D2D1::ColorF::White), 512, 512);

		// 4. 判定結果
		judgeResultText = AddUI<sf::ui::TextImage>();
		judgeResultText->transform.SetPosition(Vector3(810, 100, 0));
		judgeResultText->transform.SetScale(Vector3(8, 2, 0));
		judgeResultText->Create(
			context,
			L"", // 最初は空
			L"851ゴチカクット",
			120.0f,
			D2D1::ColorF(D2D1::ColorF::White),
			1024, 256
		);

		// =========================================================
		// 初期化
		// =========================================================
		if (auto actor = actorRef.Target()) {
			if (auto manager = actor->GetComponent<EffectManager>()) {
				effectManager = manager;
				effectManager->Initialize(this, &textureHitEffect);
			}
			noteManager = actor->GetComponent<NoteManager>();
		}

		UpdateJacketImage();
		updateCommand.Bind(std::bind(&TestCanvas::Update, this, std::placeholders::_1));
	}

	void TestCanvas::Update(const sf::command::ICommand&)
	{
		countUpTimer += sf::Time::DeltaTime();

		UpdateTimerDisplay(countUpTimer);

		int combo = JudgeStatsService::GetCombo();
		UpdateComboDisplay(combo);
		UpdateJudgeCountDisplay();

		// Updateでの判定結果更新は削除済み
	}

	void TestCanvas::SpawnHitEffect(float x, float y, float scale, float duration, const Color& color)
	{
		if (!effectManager.isNull()) {
			effectManager->Spawn(x, y, scale, duration, color);
		}
	}

	// ---------------------------------------------------------
	// 表示更新関数
	// ---------------------------------------------------------

	void TestCanvas::UpdateTimerDisplay(float time)
	{
		if (!timerText) return;
		wchar_t buf[32];
		swprintf_s(buf, L"%.2f", time);
		timerText->SetText(buf);
	}

	void TestCanvas::UpdateComboDisplay(int combo)
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

	void TestCanvas::UpdateJudgeCountDisplay()
	{
		if (!judgeStatsText) return;

		int p = JudgeStatsService::GetCount(JudgeResult::Perfect);
		int gr = JudgeStatsService::GetCount(JudgeResult::Great);
		int go = JudgeStatsService::GetCount(JudgeResult::Good);
		int m = JudgeStatsService::GetCount(JudgeResult::Miss);

		wchar_t buf[256];
		swprintf_s(buf,
			L"PERFECT: %d\n"
			L"GREAT:   %d\n"
			L"GOOD:    %d\n"
			L"MISS:    %d",
			p, gr, go, m);

		judgeStatsText->SetText(buf);
	}

	// ★関数名を SetJudgeImage に戻しました
	// 名前はImageですが、中身はテキストを更新します
	void TestCanvas::SetJudgeImage(JudgeResult result)
	{
		// ★nullptrチェック (これがないとクラッシュします)
		if (!judgeResultText) return;

		switch (result) {
		case JudgeResult::Perfect:
			judgeResultText->SetText(L"PERFECT!!");
			judgeResultText->material.SetColor({ 1.0f, 0.8f, 0.0f, 1.0f }); // 金
			break;

		case JudgeResult::Great:
			judgeResultText->SetText(L"GREAT!");
			judgeResultText->material.SetColor({ 1.0f, 0.4f, 0.7f, 1.0f }); // ピンク
			break;

		case JudgeResult::Good:
			judgeResultText->SetText(L"GOOD");
			judgeResultText->material.SetColor({ 0.2f, 1.0f, 0.2f, 1.0f }); // 緑
			break;

		case JudgeResult::Miss:
			judgeResultText->SetText(L"MISS...");
			judgeResultText->material.SetColor({ 0.6f, 0.6f, 0.6f, 1.0f }); // 灰
			break;

		case JudgeResult::None:
		default:
			// Noneの時は何も表示しない等の処理
			// judgeResultText->SetText(L"");
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
		}

		if (Jacket) {
			Jacket->material.texture = loadSuccess ? &textureJacket : &textureDefaultJacket;
		}
	}

	void TestCanvas::DestroyEffect(sf::ui::Image* effect) {
		if (!effect) return;
		uis.remove(effect);
		delete effect;
	}

} // namespace app::test
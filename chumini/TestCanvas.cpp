#include "TestCanvas.h"
#include <sstream>
#include <iomanip> // 追加
#include "Time.h"
#include "NoteManager.h"
#include "TestScene.h"

void app::test::TestCanvas::Begin()
{
	//基底クラスのBeginを必ず呼び出す必要があります
	sf::ui::Canvas::Begin();

	//テクスチャの読み込み
    textureJacket.LoadTextureFromFile("Assets\\Texture\\GOODTEK.png");
	textureDefaultJacket.LoadTextureFromFile("Assets\\Texture\\DefaultJacket.png");
	textureOk.LoadTextureFromFile("Assets\\Texture\\ok.png");
	textureCombo.LoadTextureFromFile("Assets\\Texture\\combo.png");

	//UIの追加
	Jacket = AddUI<sf::ui::Image>();
	//auto Combo = AddUI<sf::ui::Image>();

	//座標の設定
	Jacket->transform.SetPosition(Vector3(430, 400, 0));
	Jacket->transform.SetScale(Vector3(3, 3, 0));
	Jacket->material.texture = &textureJacket;
	//Jacket->material.texture = &textureDefaultJacket;

	/*Combo->transform.SetPosition(Vector3(470,-100, 0));
	Combo->transform.SetScale(Vector3(1.3, 0.8, 0));
	Combo->material.texture = &textureCombo;*/


	texturePerfect.LoadTextureFromFile("Assets\\Texture\\perfect.png");
	textureGreat.LoadTextureFromFile("Assets\\Texture\\great.png");
	textureGood.LoadTextureFromFile("Assets\\Texture\\good.png");
	textureMiss.LoadTextureFromFile("Assets\\Texture\\miss.png");

	judgeImage = AddUI<sf::ui::Image>();
	judgeImage->transform.SetPosition(Vector3(800, 100, 0));
	judgeImage->transform.SetScale(Vector3(3, 1, 0));
	judgeImage->material.texture = &textureOk; // 初期画像

	textureNumber.LoadTextureFromFile("Assets/Texture/numbers.png"); // スプライトシート

	InitializeTimerDisplay();
	InitializeComboDisplay();

	//DrawNumber(678, 300, -200, digitWidth, digitHeight, sheetWidth, sheetHeight, &textureNumber);

	// 同じActorのNoteManagerを取得
	if (auto actor = actorRef.Target()) {
		noteManager = actor->GetComponent<NoteManager>();
	}


	UpdateJacketImage();

	updateCommand.Bind(std::bind(&TestCanvas::Update, this, std::placeholders::_1));
}

void app::test::TestCanvas::InitializeTimerDisplay()
{
	// 必要な桁数分のImageオブジェクトを事前作成
	for (int i = 0; i < MAX_TIMER_DIGITS; ++i) {
		auto img = AddUI<sf::ui::Image>();
		img->transform.SetPosition(Vector3(300 + i * (digitWidth - 60), -200, 0));
		img->transform.SetScale(Vector3(0.3f, 0.3f, 0));
		img->material.texture = &textureNumber;
		img->SetVisible(false); // 初期は非表示
		timerDigits.push_back(img);
	}
}

void app::test::TestCanvas::InitializeComboDisplay()
{
	// コンボ表示用のImageオブジェクトを4桁分作成
	for (int i = 0; i < MAX_COMBO_DIGITS; ++i) {
		auto img = AddUI<sf::ui::Image>();
		img->transform.SetPosition(Vector3(250 + i * (digitWidth - 60), -100, 0)); // コンボ表示位置
		img->transform.SetScale(Vector3(0.5f, 0.5f, 0)); // タイマーより少し大きく
		img->material.texture = &textureNumber;
		img->SetVisible(false); // 初期は非表示
		comboDigits.push_back(img);
	}
}

void app::test::TestCanvas::Update(const sf::command::ICommand&)
{
	countUpTimer += sf::Time::DeltaTime();

	// 小数点第2位までの文字列に変換
	std::ostringstream oss;
	oss << std::fixed << std::setprecision(2) << countUpTimer;
	std::string timerStr = oss.str();

	// 既存のImageオブジェクトを更新（新規作成しない）
	UpdateTimerDisplay(timerStr);

	if (noteManager) {
		int combo = noteManager->GetCurrentCombo();
		static int lastCombo = -1; // 前回のコンボを記録

		// コンボが変わった時だけログ出力
		if (combo != lastCombo) {
			std::ostringstream comboOss;
			comboOss << "Canvas Update: Combo=" << combo << " (was " << lastCombo << ")";
			sf::debug::Debug::Log(comboOss.str());
			lastCombo = combo;
		}

		UpdateComboDisplay(combo);
	}
	else {
		// NoteManagerが取得できていない場合
		sf::debug::Debug::Log("Canvas Update: noteManager is null!");
	}
}


void app::test::TestCanvas::UpdateTimerDisplay(const std::string& str)
{
	// まず全ての桁を非表示に
	for (auto* img : timerDigits) {
		img->SetVisible(false);
	}

	// 文字列の長さ分だけ表示を更新
	size_t displayCount = std::min(str.size(), static_cast<size_t>(MAX_TIMER_DIGITS));

	for (size_t i = 0; i < displayCount; ++i) {
		char ch = str[i];
		int digit = -1;
		bool isDot = false;

		if (ch >= '0' && ch <= '9') {
			digit = ch - '0';
		}
		else if (ch == '.') {
			isDot = true;
		}
		else {
			continue;
		}

		// UV計算
		float u0, u1, v0 = 0.0f, v1 = digitHeight / sheetHeight;
		if (!isDot) {
			u0 = (digit * digitWidth) / sheetWidth;
			u1 = ((digit + 1) * digitWidth) / sheetWidth;
		}
		else {
			int dotIndex = 10; // 小数点のインデックス
			u0 = (dotIndex * digitWidth) / sheetWidth;
			u1 = ((dotIndex + 1) * digitWidth) / sheetWidth;
		}

		// 既存のImageのUVを更新
		timerDigits[i]->SetUV(u0, v0, u1, v1);
		timerDigits[i]->SetVisible(true);
	}
}

void app::test::TestCanvas::UpdateComboDisplay(int combo)
{
	// まず全ての桁を非表示に
	for (auto* img : comboDigits) {
		img->SetVisible(false);
	}

	// コンボが負の場合のみ何も表示しない
	if (combo < 0) {
		return;
	}

	// コンボを4桁に制限
	if (combo > 9999) {
		combo = 9999;
	}

	// 数値を文字列に変換
	std::string comboStr = std::to_string(combo);

	// 右詰めで表示するため、開始位置を計算
	int startIndex = MAX_COMBO_DIGITS - comboStr.size();

	for (size_t i = 0; i < comboStr.size(); ++i) {
		int digit = comboStr[i] - '0'; // 0～9

		// UV計算
		float u0 = (digit * digitWidth) / sheetWidth;
		float u1 = ((digit + 1) * digitWidth) / sheetWidth;
		float v0 = 0.0f;
		float v1 = digitHeight / sheetHeight;

		// 既存のImageのUVを更新
		int displayIndex = startIndex + i;
		comboDigits[displayIndex]->SetUV(u0, v0, u1, v1);
		comboDigits[displayIndex]->SetVisible(true);
	}
}


void app::test::TestCanvas::DrawNumber(int number, float x, float y, float digitWidth, float digitHeight, float sheetWidth, float sheetHeight, sf::Texture* numberTexture)
{
	std::string str = std::to_string(number);
	for (size_t i = 0; i < str.size(); ++i) {
		// 1. 桁ごとの数字を取得
		int digit = str[i] - '0'; // 0～9

		// 2. スプライトシート上の UV 算出（正規化範囲 0.0 ～ 1.0）
		float u0 = (digit * digitWidth) / sheetWidth;       // 左端のU
		float u1 = ((digit + 1) * digitWidth) / sheetWidth; // 右端のU
		float v0 = 0.0f;                                    // 上端のV
		float v1 = digitHeight / sheetHeight;               // 下端のV（今回は 100/100 = 1.0）

		// 3. Image を作成して位置/スケール/テクスチャを設定
		auto img = AddUI<sf::ui::Image>();
		img->transform.SetPosition(Vector3(
			x + i * (digitWidth - 60),   // 横並びの間隔
			y,
			0
		));
		img->transform.SetScale(Vector3(0.3f, 0.3f, 0));  // 適切なスケール
		img->material.texture = numberTexture;

		// 4. 正しい順序で UV座標を設定: (左, 上, 右, 下)

		std::ostringstream oss;
		oss
			<< "DrawNumber: digit=" << digit
			<< "  u0=" << u0
			<< "  u1=" << u1
			<< "  v0=" << v0
			<< "  v1=" << v1;
		sf::debug::Debug::Log(oss.str());

		img->SetUV(u0, v0, u1, v1);
		//img->SetUV(0.0f, 0.0f, 0.5f, 1.0f);
	}
}

void app::test::TestCanvas::SetJudgeImage(JudgeResult result)
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

void app::test::TestCanvas::UpdateJacketImage() {
    // 所有アクターからシーンを取得
    auto actor = actorRef.Target();
    if (!actor) return;
    
    auto& scene = actor->GetScene();
    auto* testScene = dynamic_cast<TestScene*>(&scene);
    if (!testScene) return;

    // 選択された楽曲情報を取得
    const SongInfo& selectedSong = testScene->GetSelectedSong();
    
    // ジャケット画像を読み込み
    bool loadSuccess = false;
    if (!selectedSong.jacketPath.empty()) {
        loadSuccess = textureJacket.LoadTextureFromFile(selectedSong.jacketPath);
        
        std::ostringstream oss;
        oss << "Loading jacket: " << selectedSong.jacketPath << " - " 
            << (loadSuccess ? "Success" : "Failed");
        sf::debug::Debug::Log(oss.str());
    }

    // ジャケット画像を更新
    if (Jacket) {
        Jacket->material.texture = loadSuccess ? &textureJacket : &textureDefaultJacket;
    }
}
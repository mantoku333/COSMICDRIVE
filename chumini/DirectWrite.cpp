#include "DirectWriteCustomFont.h"
#include "App.h"
#include "D3D.h"
#include "DirectWrite.h"

sf::dx::SwapChain* gSwapChain = nullptr;

void DirectWrite()
{

	// フォントデータ
	FontData data;

	// DirectWrite描画クラス
	DirectWriteCustomFont* Write;

	// DirectWriteCustomFontクラスの生成
	Write = new DirectWriteCustomFont(&data);

	extern sf::dx::SwapChain* gSwapChain;

	// 初期化（SwapChainの取得は適宜お願いします）
	if (gSwapChain) {
		Write->Init(gSwapChain->GetSwapChain());
	}
	else {
		//えらーーーーー
	}

	//// 日本語ロケールのフォント名を取得
	//Write->GetFontFamilyName(Write->fontCollection.Get(), L"ja-JP");

	//// フォントデータを改変
	//data.fontSize = 60;
	//data.fontWeight = DWRITE_FONT_WEIGHT_ULTRA_BLACK;
	//data.Color = D2D1::ColorF(D2D1::ColorF::Red);
	//data.font = Write->GetFontName(3);

	//// フォントをセット
	//Write->SetFont(data);



	//// 描画（実際はDrawの方で呼び出してください）
	//Write->DrawString(
	//	"ここはタイトル画面です",
	//	D2D1::RectF(90.0f, 90.0f, 1000.0f, 200.0f),  // 左,上,右,下（適当に広めでOK）
	//	D2D1_DRAW_TEXT_OPTIONS_NONE,
	//	true  // 影を出したいなら true / いらなければ false
	//);

	//// メモリ解放
	//delete Write;
	//Write = nullptr;
}
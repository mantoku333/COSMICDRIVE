#include "App.h"
#include "DirectX11.h"
#include "ResidentScene.h"
#include "Live2DManager.h"

app::Application* app::Application::main = nullptr;

int app::Application::width = 1920;
int app::Application::height = 1080;

app::Application::Application()
{
	sf::debug::Debug::Log("アプリケーション開始");

	if (main == nullptr)
	{
		main = this;
	}
	else
	{
		sf::debug::Debug::LogError("複数のアプリケーションが同時に存在しています");
	}

	sf::debug::Debug::WriteLogFile();

	OnApplicationExit += sf::IScene::DestroyFromOnApplicaitonExit;

	fpsFile.Clean();
}

app::Application::~Application()
{
	if (main == this)
	{
		main = nullptr;
	}

	sf::debug::Debug::Log("アプリケーション終了");

	exit = false;
}

void app::Application::Run()
{
	try
	{
		sf::debug::Debug::Log("アプリケーションの初期化を開始します");
		Init();
	}
	catch (const std::exception& log)
	{

		sf::debug::Debug::LogError(log.what());
		sf::debug::Debug::LogError("ゲームを強制終了します");

		return;
	}


	Loop();

	UnInit();
}

void app::Application::ActivateScene(sf::IScene* scene)
{
	{
		std::lock_guard<std::mutex> lock(scenesMtx);
		activeScene.push_back(scene);
		sf::debug::Debug::LogEngine("シーンがアクティベートされました:" + std::string(typeid(*scene).name()));
	}
}

void app::Application::DeActivateScene(const sf::IScene* scene)
{
	{
		std::lock_guard<std::mutex> lock(scenesMtx);
		auto it = std::find(activeScene.begin(), activeScene.end(), scene);

		if (it != activeScene.end())
		{
			activeScene.erase(it);
		}
	}
}

void app::Application::Exit()
{
	exit = true;
}

void app::Application::Init()
{
	//ウィンドウ作成
	CreateGameWindow();

	//サウンドリソースの初期化
	sf::sound::SoundResource::Init();

	//入力の初期化
	SInput::Init();

	//時間の初期化
	sf::Time::Init();

	//DirectXの初期化
	sf::dx::DirectX11::Init();
	sf::dx::DirectX11::Instance()->Create(gameWindow.hwnd);

#ifdef USEGUI
	//GUIの初期化
	sf::gui::GUI::Init(
		gameWindow.hwnd,
		sf::dx::DirectX11::Instance()->GetMainDevice().GetDevice(),
		sf::dx::DirectX11::Instance()->GetMainDevice().GetContext()
	);
#endif

	//スカイボックスの初期化
	skybox.Init();

	Live2DManager::GetInstance()->Initialize();
	live2DModel = new AppModel();
	live2DModel->LoadAssets("Assets/Live2D/Hiyori/", "Hiyori.model3.json");

	//ResidentSceneを読み込む(常に存在するシーン)
	auto scene = ResidentScene::StandbyScene();

	while (1)
	{
		//ResidentSceneの読み込みを待つ
		if (ResidentScene::Standby())
		{
			break;
		}
	}

	//ResidentSceneをアクティベート状態にする
	scene->Activate();
}

void app::Application::UnInit()
{
	for (auto& i : activeScene) {
		delete i;
	}
	if (live2DModel) {
		delete live2DModel;
		live2DModel = nullptr;
	}
	Live2DManager::GetInstance()->Release();

	OnApplicationExit();

#ifdef USEGUI
	sf::gui::GUI::UnInit();
#endif

	sf::sound::SoundResource::UnInit();
	sf::dx::DirectX11::UnInit();

	SInput::UnInit();
}

void app::Application::Loop()
{
	//sf::Time::SetFPS(0);
	sf::Time::SetFPS(60);

	sf::debug::Debug::Log("ゲームループです");
	MSG msg{};
	while (1)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) == TRUE)
		{
			if (msg.message == WM_QUIT) { break; }

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			if (sf::Time::Update())
			{
				//入力の更新
				SInput::Instance().Update();

				if (live2DModel) {
					live2DModel->Update();
				}

				//DirectX11の取得
				sf::dx::DirectX11* dx11 = sf::dx::DirectX11::Instance();

				//GPUにシステム情報を送信する
				static float t = 0.0f;
				t += sf::Time::DeltaTime();
				System sys;
				sys.time.x = t;
				sys.screenSize = DirectX::XMFLOAT2A(1920, 1080);
				dx11->systemBuffer.SetGPU(sys, dx11->GetMainDevice());

#ifdef USEGUI
				//GUI描画
				sf::gui::GUI::Begin();

				//デバッグライン
//				const float l = 5.0f;
//				// X 軸
//				sf::debug::Debug::DrawLine({ -l,0,0 }, { +l,0,0 }, { 1,0,0,1 });
//				// Y 軸
//				sf::debug::Debug::DrawLine({ 0,-l,0 }, { 0,+l,0 }, { 0,1,0,1 });
//				// Z 軸
//				sf::debug::Debug::DrawLine({ 0,0,-l }, { 0,0,+l }, { 0,0,1,1 });
#endif

				//RTVの塗りつぶし
				dx11->SetViewPort(1920, 1080);

				//全コマンドの実装
				sf::command::ICommand::CallAll();

				//影の描画
				DrawShadow();

				//カメラ情報をGPUに転送
				sf::Camera::SetGPU();

				//次のレンダリングバッファに切り替え
				dx11->SetNextRenderingDoubleBuffer3D();

				//スカイボックス描画設定
				dx11->SetRenderingDoubleBuffer3DSkyBox();

				//スカイボックス描画
				skybox.Draw();

				//3D描画開始
				dx11->SetRenderingDoubleBuffer3D();

				//シャドウマップをGPUに転送
				ID3D11ShaderResourceView* srv = nullptr;
				dx11->GetMainDevice().GetContext()->PSSetShaderResources(3, 1, &srv);
				dx11->shadowRTV.SetTexture(dx11->GetMainDevice(), 3);

				//メッシュの描画
				sf::Mesh::DrawAll();

				for (auto* scene : activeScene) {
					scene->Draw();
				}

				if (live2DModel) {
					// 行列の作成（単位行列＝画面中央）
					Live2D::Cubism::Framework::CubismMatrix44 matrix;
					matrix.LoadIdentity();

					// 必要ならサイズ調整 (1.0だと画面いっぱいすぎるかも)
					// matrix.Scale(1.0f, 1.0f); 

					// 描画実行
					// GetDevice()などはご自身のエンジンに合わせて取得してください
					live2DModel->Draw(
						dx11->GetMainDevice().GetDevice(),
						dx11->GetMainDevice().GetContext(),
						matrix
					);

					// ★重要：Live2Dは描画ステート（ブレンドやシェーダー）を書き換えます。
					// 次の処理（デバッグライン描画や2D描画）で表示がおかしくなる場合は、
					// ここで「レンダリングステートを元に戻す処理」が必要です。
					// いったんこのまま動かしてみましょう。
				}

				//デバッグラインの描画
				sf::debug::Debug::DrawLog();

				//2D描画開始
				dx11->SetRenderingDoubleBuffer2D();

				//UI描画
				sf::ui::Canvas::DrawCanvasies();

				//オンスクリーン描画開始
				dx11->OnScreenRendering();

				bool exitFg = false;
#ifdef USEGUI
				exitFg = OnGUI();
#endif
				//ダブルバッファ切り替え
				dx11->Flip();

				//アクターの破棄
				sf::Actor::DestroyActors();

				//シーンの破棄
				sf::IScene::DestroyScenes();

				if (exitFg)break;

				if (exit)break;

				double fps = sf::Time::GetFPS();
				std::string strFps = std::to_string(fps);

				fpsFile.Write({ strFps });

				if (fps < 30)
				{
					sf::debug::Debug::LogEngine("FPSが低下しています:" + strFps);
				}
			}
		}
	}
}

void app::Application::DrawShadow()
{
	//DX11の取得
	sf::dx::DirectX11* dx11 = sf::dx::DirectX11::Instance();

	//塗りつぶし
	float color[] = { 0,0,0,1 };
	dx11->GetMainDevice().GetContext()->ClearRenderTargetView(dx11->shadowRTV.GetRTV(), color);
	ID3D11RenderTargetView* rtv[]
	{
		dx11->shadowRTV.GetRTV(),
	};

	//RTV,DSVをGPUにセット
	dx11->GetMainDevice().GetContext()->ClearDepthStencilView(dx11->depth, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	dx11->GetMainDevice().GetContext()->OMSetRenderTargets(ARRAYSIZE(rtv), rtv, dx11->depth);

	//影用ピクセルシェーダーをGPUにセット
	dx11->psShadow.SetGPU(dx11->GetMainDevice());

	//カメラ情報をGPUに転送
	sf::Camera::shadow->SetGPUShadow();

	{
		//ピクセルシェーダーの切り替えをロック
		sf::dx::shader::PixelShader::Lock();

		//シャドウマップに描画
		sf::Mesh::DrawShadowAll();

		//ピクセルシェーダーの切り替えを案ロック
		sf::dx::shader::PixelShader::UnLock();
	}

	//ImGui::Begin("Shadow");

	//ImGui::Image(ImTextureID(dx11->shadowRTV.GetSRV()), ImVec2(400, 400));

	//ImGui::End();
}

bool app::Application::OnGUI()
{
	//DirectX11の取得
	sf::dx::DirectX11* dx11 = sf::dx::DirectX11::Instance();


	sf::debug::Debug::TraceLog();

	for (auto& i : activeScene) {
		i->OnGUI();
	}

	ImGui::Begin("GBuffer");

	ImVec2 s;
	s.x = 200;
	s.y = 200;

	ImGui::Text(sf::ConvertToUTF8("レンダリングバッファ").c_str());

	ImGui::Text(sf::ConvertToUTF8("3Dベースカラー").c_str());
	ImGui::Image(
		(ImTextureID)(dx11->rb3d.Get().GetBaseColorRTV().GetSRV()),
		s);

	ImGui::Text(sf::ConvertToUTF8("3D法線").c_str());
	ImGui::Image(
		(ImTextureID)(dx11->rb3d.Get().GetNormalRTV().GetSRV()),
		s);
	ImGui::Text(sf::ConvertToUTF8("3Dアルファ深度").c_str());
	ImGui::Image(
		(ImTextureID)(dx11->rb3d.Get().GetAlphaRTV().GetSRV()),
		s);
	ImGui::Text(sf::ConvertToUTF8("輝度").c_str());
	ImGui::Image(
		(ImTextureID)(dx11->rb3d.Get().GetLuminanceRTV().GetSRV()),
		s);
	ImGui::Text(sf::ConvertToUTF8("DownX").c_str());
	ImGui::Image(
		(ImTextureID)(dx11->downX.GetSRV()),
		s);
	ImGui::Text(sf::ConvertToUTF8("DownY").c_str());
	ImGui::Image(
		(ImTextureID)(dx11->downY.GetSRV()),
		s);
	ImGui::Text(sf::ConvertToUTF8("2Dベースカラー").c_str());
	ImGui::Image(
		(ImTextureID)(dx11->rb2d.GetBaseColorRTV().GetSRV()),
		s);
	ImGui::Text(sf::ConvertToUTF8("2Dアルファ値").c_str());
	ImGui::Image(
		(ImTextureID)(dx11->rb2d.GetAlphaRTV().GetSRV()),
		s);

	ImGui::End();

	ImGui::Begin("System");

	bool exitFg = false;
	if (ImGui::Button("Exit"))
	{
		int i = MessageBox(nullptr, "実行を停止しますか？", "Exit", MB_YESNO);
		if (i == IDYES)
		{
			exitFg = true;
		}
	}

	if (ImGui::Button(sf::ConvertToUTF8("動的コンパイル").c_str()))
	{
		//動的コンパイルを行うにはF5を押して下さい
		__debugbreak();  // Visual Studioのデバッガで停止します
	}

	float c = 0;
	for (int i = 60 - 1; i > 0; i--)
	{
		fps[i] = fps[i - 1];
		c += fps[i];
	}
	fps[0] = sf::Time::GetFPS();
	c += fps[0];
	c /= 60.0f;
	ImGui::Text(sf::ConvertToUTF8(std::string("実際のFPS:" + std::to_string(sf::Time::GetFPS()))).c_str());
	ImGui::Text(sf::ConvertToUTF8(std::string("平均FPS:" + std::to_string(c))).c_str());

	ImGui::End();

	sf::gui::GUI::End();

	return exitFg || exit;
}

bool app::Application::CreateGameWindow()
{
	gameWindow.wc.lpfnWndProc = WndProc;
	gameWindow.windowName = "Window1";
	gameWindow.className = "Class1";

	return gameWindow.Create(WS_POPUP);
	return gameWindow.Create();
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler
(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT app::Application::WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wp, lp)) {
		return true;
	}

	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_KEYDOWN:			//何かしらのキーが押されたら
		SInput::Instance().SetKeyDown(static_cast<int>(wp));
		break;

	case WM_KEYUP:				//何かしらのキーが離された時
		SInput::Instance().SetKeyUp(static_cast<int>(wp));
		break;
	case WM_LBUTTONDOWN:
		SInput::Instance().SetMouseDown(0);
		break;
	case WM_RBUTTONDOWN:
		SInput::Instance().SetMouseDown(1);
		break;
	case WM_LBUTTONUP:
		SInput::Instance().SetMouseUp(0);
		break;
	case WM_RBUTTONUP:
		SInput::Instance().SetMouseUp(1);
		break;

	default:
		break;
	}
	return DefWindowProc(hWnd, msg, wp, lp);
}

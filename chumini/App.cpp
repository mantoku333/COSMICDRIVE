#include "App.h"
#include "DirectX11.h"
#include "ResidentScene.h"
#include "Config.h"

app::Application* app::Application::main = nullptr;

int app::Application::width = 1920;
int app::Application::height = 1080;

app::Application::Application()
{
	sf::debug::Debug::Log("Application Start");

	if (main == nullptr)
	{
		main = this;
	}
	else
	{
		sf::debug::Debug::LogError("Multiple applications exist");
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

	sf::debug::Debug::Log("Application End");

	exit = false;
}

void app::Application::Run()
{
	try
	{
		sf::debug::Debug::Log("Starting Application Init");
		Init();
	}
	catch (const std::exception& log)
	{

		sf::debug::Debug::LogError(log.what());
		sf::debug::Debug::LogError("Force quitting game");

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
		sf::debug::Debug::LogEngine("Scene Activated:" + std::string(typeid(*scene).name()));
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

	// Load Config
	app::test::LoadConfig();

	//時間の初期化
	sf::Time::Init();

	//JobSystemの初期化
	sf::jobsystem::JobSystem::Create(std::thread::hardware_concurrency() - 1);

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

	OnApplicationExit();

#ifdef USEGUI
	sf::gui::GUI::UnInit();
#endif

	sf::sound::SoundResource::UnInit();
	sf::dx::DirectX11::UnInit();
	sf::jobsystem::JobSystem::Destroy();

	SInput::UnInit();
}

void app::Application::Loop()
{
	//sf::Time::SetFPS(0);
	sf::Time::SetFPS(60);

	sf::debug::Debug::Log("Game Loop");
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
					// Parallel Update
					sf::jobsystem::JobSystem::Instance()->addJob([this] {
						live2DModel->Update();
					});
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

				// Wait for Live2D Update
				if (live2DModel) {
					sf::jobsystem::JobSystem::Instance()->waitForAllJobs();
				}

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

				//デバッグラインの描画
				sf::debug::Debug::DrawLog();

				//2D描画開始
				dx11->SetRenderingDoubleBuffer2D();

				//UI描画
				sf::ui::Canvas::DrawCanvasies();



				//オンスクリーン描画開始
				dx11->OnScreenRendering();

				// 最前面描画 (Live2Dなど)
				for (auto* scene : activeScene) {
					scene->DrawOverlay();
				}

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
					sf::debug::Debug::LogEngine("FPS Dropping:" + strFps);
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

	ImGui::Text((const char*)u8"\u30EC\u30F3\u30C0\u30EA\u30F3\u30B0\u30D0\u30C3\u30D5\u30A1");

	ImGui::Text((const char*)u8"3D\u30D9\u30FC\u30B9\u30AB\u30E9\u30FC");
	ImGui::Image(
		(ImTextureID)(dx11->rb3d.Get().GetBaseColorRTV().GetSRV()),
		s);

	ImGui::Text((const char*)u8"3D\u6CD5\u7DDA");
	ImGui::Image(
		(ImTextureID)(dx11->rb3d.Get().GetNormalRTV().GetSRV()),
		s);
	ImGui::Text((const char*)u8"3D\u30A2\u30EB\u30D5\u30A1\u6DF1\u5EA6");
	ImGui::Image(
		(ImTextureID)(dx11->rb3d.Get().GetAlphaRTV().GetSRV()),
		s);
	ImGui::Text((const char*)u8"\u881D\u5EA6");
	ImGui::Image(
		(ImTextureID)(dx11->rb3d.Get().GetLuminanceRTV().GetSRV()),
		s);
	ImGui::Text("DownX");
	ImGui::Image(
		(ImTextureID)(dx11->downX.GetSRV()),
		s);
	ImGui::Text("DownY");
	ImGui::Image(
		(ImTextureID)(dx11->downY.GetSRV()),
		s);
	ImGui::Text((const char*)u8"2D\u30D9\u30FC\u30B9\u30AB\u30E9\u30FC");
	ImGui::Image(
		(ImTextureID)(dx11->rb2d.GetBaseColorRTV().GetSRV()),
		s);
	ImGui::Text((const char*)u8"2D\u30A2\u30EB\u30D5\u30A1\u5024");
	ImGui::Image(
		(ImTextureID)(dx11->rb2d.GetAlphaRTV().GetSRV()),
		s);

	ImGui::End();

	ImGui::Begin("System");

	bool exitFg = false;
	if (ImGui::Button("Exit"))
	{
		// Unicode escape for Japanese "Execute Stop?"
		int i = MessageBoxW(nullptr, L"\u5B9F\u884C\u3092\u505C\u6B62\u3057\u307E\u3059\u304B\uFF1F", L"Exit", MB_YESNO);
		if (i == IDYES)
		{
			exitFg = true;
		}
	}

	if (ImGui::Button((const char*)u8"\u52D5\u7684\u30B3\u30F3\u30D1\u30A4\u30EB"))
	{
		// Press F5 for dynamic compile
		__debugbreak();  // Visual Studio break
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
	// "Actual FPS: %f" and "Avg FPS: %f" in Unicode escape
	ImGui::Text((const char*)u8"\u5B9F\u969B\u306EFPS: %f", sf::Time::GetFPS());
	ImGui::Text((const char*)u8"\u5E73\u5747FPS: %f", c);

	ImGui::End();

	sf::gui::GUI::End();

	return exitFg || exit;
}

bool app::Application::CreateGameWindow()
{
	gameWindow.wc.lpfnWndProc = WndProc;
	gameWindow.windowName = "COZMIC DRIVE";
	gameWindow.className = "Class1";

	bool result = gameWindow.Create(WS_POPUP);

	// Force disable TOPMOST
	SetWindowPos(gameWindow.hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	return result;
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

	case WM_KEYDOWN:			//Key Down
		SInput::Instance().SetKeyDown(static_cast<int>(wp));
		break;

	case WM_KEYUP:				//Key Up
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

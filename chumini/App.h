#pragma once
#include "sf.h"
#include "Scene.h"
#include "Time.h"
#include "SInput.h"
#include "SkyBox.h"
#include "AppModel.h"

namespace app
{
	//ゲームのアプリケーション
	class Application
	{
	public:
		Application();
		~Application();

		/// <summary>
		/// メインアプリケーション
		/// </summary>
		/// <returns></returns>
		static Application* GetMain() { return main; }

		/// <summary>
		/// アプリケーションの実行
		/// </summary>
		void Run();

		/// <summary>
		/// アクティベートのシーンを追加
		/// </summary>
		/// <param name="scene"></param>
		void ActivateScene(sf::IScene* scene);

		/// <summary>
		/// アクティベートのシーンを削除
		/// </summary>
		/// <param name="scene"></param>
		void DeActivateScene(const sf::IScene* scene);

		/// <summary>
		/// ウィンドウの幅を取得
		/// </summary>
		static int GetWidth() { return width; }

		/// <summary>
		/// ウィンドウの高さを取得
		/// </summary>
		static int GetHeight() { return height; }


		/// <summary>
		/// アプリケーションの終了
		/// </summary>
		void Exit();

	private:
		void Init();
		void UnInit();
		void Loop();

		void DrawShadow();

		bool OnGUI();

		bool CreateGameWindow();

		static LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);

	public:
		sf::jobsystem::JobSystem jobSystem;

		sf::del::VDelegate OnApplicationExit;

		static int width;
		static int height;

	private:
		static Application* main;

		float fps[60]{ 
			60,60,60,60,60,60,60,60,60,60,
			60,60,60,60,60,60,60,60,60,60,
			60,60,60,60,60,60,60,60,60,60,
			60,60,60,60,60,60,60,60,60,60,
			60,60,60,60,60,60,60,60,60,60,
			60,60,60,60,60,60,60,60,60,60,
		};

		//ゲームウィンドウ
		sf::window::Window gameWindow;

		SkyBox skybox;

		//アクティブシーン
		std::vector<sf::IScene*> activeScene;

		std::mutex scenesMtx;

		bool exit = false;

		sf::file::TextFile fpsFile = sf::file::TextFile("log\\fps.txt");

		AppModel* live2DModel = nullptr;
	};
}
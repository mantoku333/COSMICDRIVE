#pragma once
#include "Singleton.h"
#include "MainDevice.h"
#include "OnScreenRTV.h"
#include "OffScreenRTV.h"
#include "VertexShader.h"
#include "GeometryShader.h"
#include "PixelShader.h"
#include "Sampler.h"
#include "Depth.h"
#include "BlendState.h"

#include "RenderingBuffer2D.h"
#include "RenderingBuffer3D.h"
#include "ConstBuffer.h"

#include "Debug.h"
#include "Image.h"

struct WorldMatrixBuffer
{
	DirectX::XMMATRIX mtx;
	DirectX::XMMATRIX rot;
};

struct VP
{
	DirectX::XMFLOAT4 cameraPos;
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX proj;
	DirectX::XMMATRIX shadowVP;
};

struct mtl
{
	DirectX::XMFLOAT4 diffuseColor;
	DirectX::XMFLOAT4 emissionColor;
	DirectX::XMFLOAT4 textureEnable;
};

struct cmotion
{
	DirectX::XMMATRIX matrix[400];
};

struct System
{
	DirectX::XMFLOAT4 time;
	DirectX::XMFLOAT2 screenSize;
	float glitchIntensity;
	float padding; // 16byte alignment
};


namespace sf
{
	namespace dx
	{
		//DirectXクラス(D3D/シングルトン)
		class DirectX11 :public Singleton<DirectX11>
		{
		public:
			/// <summary>
			/// DirectX11の初期化
			/// </summary>
			/// <param name="hwnd">ウィンドウハンドル</param>
			void Create(HWND hwnd);

			void SetGlitchIntensity(float intensity);
			float GetGlitchIntensity() const { return m_glitchIntensity; }

			void Flip() { mainDevice.Flip(); }

			/// <summary>
			/// メインデバイスの取得
			/// </summary>
			/// <returns></returns>
			MainDevice& GetMainDevice(){ return mainDevice; }

		private:
			float m_glitchIntensity = 0.0f;

		public:
			/// <summary>
			/// 次のレンダリングバッファを用意する
			/// </summary>
			void SetNextRenderingDoubleBuffer3D();

			/// <summary>
			/// スカイボックス描画開始設定
			/// </summary>
			void SetRenderingDoubleBuffer3DSkyBox();

			/// <summary>
			/// 3D描画開始設定
			/// </summary>
			void SetRenderingDoubleBuffer3D();

			/// <summary>
			/// 2D描画開始設定
			/// </summary>
			void SetRenderingDoubleBuffer2D();

			/// <summary>
			/// オンスクリーンレンダリング
			/// </summary>
			void OnScreenRendering();

			/// <summary>
			/// ビューポートの設定
			/// </summary>
			void SetViewPort();
			void SetViewPort(int w, int h);

			HWND GetHWND() const { return hwnd; }

		private:
			void LoadShader();

			void PostProcessing();
			void DownSamplingX();
			void DownSamplingY();

		public:
			//定数バッファ
			ConstBuffer<WorldMatrixBuffer> wBuffer;
			ConstBuffer<VP> vpBuffer;
			ConstBuffer<mtl> mtlBuffer;
			ConstBuffer<cmotion> motionBuffer;
			ConstBuffer<debug::Debug::LineData> lineBuffer;
			ConstBuffer<System> systemBuffer;
			ConstBuffer<UVRect> uvRectBuffer;

			//シェーダー
			shader::VertexShader vs3d;
			shader::VertexShader vsMotion;
			shader::VertexShader vsNone;

			shader::GeometryShader gsNone;
			shader::GeometryShader gsCube;
			shader::GeometryShader gs2d;
			shader::GeometryShader gsOnScreen;
			shader::GeometryShader gsDebugLine;
            shader::GeometryShader gsScoreGauge; // New
			shader::GeometryShader gsDownX;
			shader::GeometryShader gsDownY;

			shader::PixelShader ps3d;
			shader::PixelShader ps2d;
			shader::PixelShader psOnScreen;
			shader::PixelShader psDebugLine;
			shader::PixelShader psShadow;
			shader::PixelShader psBlur;
			shader::PixelShader psPostProcess;
            shader::PixelShader psScoreGauge; // New


		public:
			//メインデバイス
			MainDevice mainDevice;

			//オンスクリーン
			OnScreenRTV onScreen;

			//レンダリングバッファ
			rbuffer::RenderingBuffer2D rb2d;
			rbuffer::RenderingDoubleBuffer3D rb3d;
			OffScreenRTV shadowRTV;

			OffScreenRTV downX;
			OffScreenRTV downY;

			//深度
			Depth depth;

			//ディファードコンテキスト
			Context context;

			//サンプラー
			Sampler sampler;

			BlendState alphaBlend;
			BlendState addBlend;

			BlendState bs2D;

			//ウィンドウハンドル
			HWND hwnd;
		};
	}
}

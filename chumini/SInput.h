#pragma once
//---------- LIBRARY -----------
#pragma comment (lib, "xinput.lib")
#pragma comment (lib, "dinput8.lib")

//------------- INCLUDES ----------
#include <dinput.h>
#include <Xinput.h>

//------------- DEFINE ----------
#define STICK_MAX	(32767)
#define STICK_MIN	(-32768)
#define TRIGGER_MAX	(255.0f)
#define VIBRATION_MAX	(65535)	//振動最大強度
#define KEYLENGTH (256)	//キー総数

//キーボード
enum class Key {
	ESCAPE = 0x1B,
	KEY_BACK = 0x08,
	SPACE = 0x20,
	KEY_SHIFT = 0x10, // ← 0xA0から0x10に修正
	KEY_CTRL = 0x11,
	KEY_0 = '0',
	KEY_1 = '1',
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,
	KEY_A = 'A',
	KEY_B,
	KEY_C,
	KEY_D,
	KEY_E,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_I,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_M,
	KEY_N,
	KEY_O,
	KEY_P,
	KEY_Q,
	KEY_R,
	KEY_S,
	KEY_T,
	KEY_U,
	KEY_V,
	KEY_W,
	KEY_X,
	KEY_Y,
	KEY_Z,
	KEY_LEFT = 0x25,//VK_LEFT
	KEY_UP,
	KEY_RIGHT,
	KEY_DOWN,
	KEY_HANKAKU = 229,
	KEY_UNKNOWN = -1
};

//コントローラーボタン
enum class PadButton
{
	PAD_A = XINPUT_GAMEPAD_A,							//Aボタン
	PAD_B = XINPUT_GAMEPAD_B,							//Bボタン
	PAD_X = XINPUT_GAMEPAD_X,							//Xボタン
	PAD_Y = XINPUT_GAMEPAD_Y,							//Yボタン
	PAD_L_SHOULDER = XINPUT_GAMEPAD_LEFT_SHOULDER,		//LB
	PAD_R_SHOULDER = XINPUT_GAMEPAD_RIGHT_SHOULDER,		//RB
	PAD_BACK = XINPUT_GAMEPAD_BACK,						//Back
	PAD_START = XINPUT_GAMEPAD_START,					//Start
	PAD_LEFT_THUMB = XINPUT_GAMEPAD_LEFT_THUMB,			//左スティック
	PAD_RIGHT_THUMB = XINPUT_GAMEPAD_RIGHT_THUMB,		//右スティック
	PAD_UP = XINPUT_GAMEPAD_DPAD_UP,					//十字キーの上
	PAD_DOWN = XINPUT_GAMEPAD_DPAD_DOWN,				//十字キーの下
	PAD_LEFT = XINPUT_GAMEPAD_DPAD_LEFT,				//十字キーの左
	PAD_RIGHT = XINPUT_GAMEPAD_DPAD_RIGHT,				//十字キーの右
};

//コントローラースティック
enum class PadStick
{
	PAD_L_X,					//左スティックX軸
	PAD_L_Y,					//左スティックY軸
	PAD_R_X,					//左スティックX軸
	PAD_R_Y,					//左スティックY軸
	PAD_L_TRIGGER,				//左トリガー
	PAD_R_TRIGGER,				//右トリガー
};

//キーボード入力クラス
class KeyBoard
{
public:
	KeyBoard();
	~KeyBoard();

	//更新処理
	virtual void Update();

	//ウィンドウプロシージャ関数から呼び出すべき関数
	//押下状態のセット
	void SetKeyDown(int keycord);

	//押上状態のセット
	void SetKeyUp(int keycord);


	//押下状態の検知
	bool GetKey(Key keycord) const;

	//トリガー入力の検知
	bool GetKeyDown(Key keycord) const;

	//押上状態の検知
	bool GetKeyUp(Key keycord) const;

private:
	static bool GetKey(const bool* _state, Key keycord);

private:
	//キー状態の記憶領域
	bool keyState[KEYLENGTH] = { false };

	//今のフレームのキー状態
	bool state[KEYLENGTH] = { false };

	//1フレーム前のキー状態
	bool oldState[KEYLENGTH] = { false };
};

//XBoxコントローラー用入力クラス
class XBoxInput
{
public:
	XBoxInput();
	virtual ~XBoxInput();

	virtual void Update();

	//コントローラー接続しているか確認する関数
	bool IsConnect();

	//ボタン入力を取得する関数
	bool GetController(PadButton button) const;

	//Down入力を取得する関数
	bool GetControllerUp(PadButton button) const;

	//Up入力を取得する関数
	bool GetControllerDown(PadButton button) const;

	//スティック入力を取得する関数
	float GetController(PadStick stick);

	//スティックのUp入力を取得する関数
	bool GetControllerUp(PadStick stick, bool plus = true, float deadLine = 0.1f);

	//スティックのDown入力を取得する関数
	bool GetControllerDown(PadStick stick, bool plus = true, float deadLine = 0.1f);

private:
	static bool GetDown(const XINPUT_STATE& state, const PadButton button);
	static float GetStick(const XINPUT_STATE& state, const PadStick stick);

private:
	bool isConnect;

	XINPUT_STATE state;
	XINPUT_STATE preState;
};

//XBoxコントローラー用出力クラス
class XBoxOutput
{
public:
	XBoxOutput();
	virtual ~XBoxOutput();

	virtual void Update();

	//振動をセットする関数
	void SetVibration(int frame, float value = 1.0f);

	//左側の振動をセットする関数
	void SetVibrationLeft(int frame, float value = 1.0f);

	//右側の振動をセットする関数
	void SetVibrationRight(int frame, float value = 1.0f);

	//振動を停止する関数
	void StopVibration();

	//左側の振動を停止する関数
	void StopVibrationLeft();

	//右側の振動を停止する関数
	void StopVibrationRight();

private:
	int lFrame;
	int rFrame;

	XINPUT_VIBRATION vibration;
};

class Mouse
{
public:
	void Update();

	void SetMouseDown(int idx);
	void SetMouseUp(int idx);
    void SetMouseWheel(float val);

	bool GetMouseDown(int idx)const;
	bool GetMouse(int idx) const;
	bool GetMouseUp(int idx)const;
    float GetMouseWheel() const;

private:
	bool state[2]{};
	bool oldState[2]{};

	bool keyState[2]{};
    float wheel = 0.0f;
    float wheelBuffer = 0.0f;
};

class SInput :public XBoxInput, public XBoxOutput, public KeyBoard, public Mouse
{
public:
	SInput();
	~SInput();

	void Update()override;

	static void Init();
	static void UnInit();

	static SInput& Instance();

	// Raw Input 初期化
	void InitRawInput(HWND hwnd);
	// Raw Input メッセージ処理
	void ProcessRawInput(LPARAM lParam);

	DWORD GetLastRawInputTime() const { return lastRawTime; }
	LARGE_INTEGER GetLastRawInputQPC() const { return lastRawQPC; }

private:
	static SInput* instance;
	DWORD lastRawTime = 0;
	LARGE_INTEGER lastRawQPC = {};
};
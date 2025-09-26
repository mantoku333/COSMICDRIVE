#include "SInput.h"

KeyBoard::KeyBoard()
{

}

KeyBoard::~KeyBoard()
{

}

void KeyBoard::Update()
{
	for (int i = 0; i < KEYLENGTH; i++) {
		oldState[i] = state[i];	//1フレーム前の情報を保存
		state[i] = keyState[i];	//今の状態を適応
	}
}

void KeyBoard::SetKeyDown(int keycord) { keyState[keycord] = true; }
void KeyBoard::SetKeyUp(int keycord) { keyState[keycord] = false; }
bool KeyBoard::GetKey(Key keycord) const { return GetKey(state, keycord); }
bool KeyBoard::GetKeyDown(Key keycord) const { return !GetKey(oldState, keycord) && GetKey(state, keycord); }
bool KeyBoard::GetKeyUp(Key keycord) const { return GetKey(oldState, keycord) && !GetKey(state, keycord); }

bool KeyBoard::GetKey(const bool* _state, Key keycord) { return _state[static_cast<int>(keycord)]; }

XBoxInput::XBoxInput()
{
	state = XINPUT_STATE();
	preState = XINPUT_STATE();

	Update();
}

XBoxInput::~XBoxInput()
{

}

void XBoxInput::Update()
{
	//前の状態を保存する
	preState = state;

	isConnect = true;

	//XInputGetState:コントローラー入力を取る関数
	//接続がないと「ERROR_DEVICE_NOT_CONNECTED」が返ってくる
	if (XInputGetState(0, &state) == ERROR_DEVICE_NOT_CONNECTED)
	{
		state = XINPUT_STATE();
		isConnect = false;
	}
}

bool XBoxInput::IsConnect() { return isConnect; }

bool XBoxInput::GetController(PadButton button) const { return GetDown(state, button); }
bool XBoxInput::GetControllerUp(PadButton button) const { return (!GetDown(state, button)) && GetDown(preState, button); }
bool XBoxInput::GetControllerDown(PadButton button) const { return GetDown(state, button) && (!GetDown(preState, button)); }

float XBoxInput::GetController(PadStick stick) { return GetStick(state, stick); }

bool XBoxInput::GetControllerUp(PadStick stick, bool plus, float deadLine)
{
	float now = GetStick(state, stick);
	float pre = GetStick(preState, stick);

	bool nowFg;
	bool preFg;

	if (plus)
	{
		nowFg = now > deadLine;
		preFg = pre > deadLine;
	}
	else
	{
		nowFg = now < -deadLine;
		preFg = pre < -deadLine;
	}

	return (!nowFg) && preFg;
}

bool XBoxInput::GetControllerDown(PadStick stick, bool plus, float deadLine)
{
	float now = GetStick(state, stick);
	float pre = GetStick(preState, stick);

	bool nowFg;
	bool preFg;

	if (plus)
	{
		nowFg = now > deadLine;
		preFg = pre > deadLine;
	}
	else
	{
		nowFg = now < -deadLine;
		preFg = pre < -deadLine;
	}

	return nowFg && (!preFg);
}

bool XBoxInput::GetDown(const XINPUT_STATE& _state, const PadButton button) { return _state.Gamepad.wButtons & static_cast<int>(button); }

float XBoxInput::GetStick(const XINPUT_STATE& state, const PadStick stick)
{
	float ret = 0.0f;
	float value = 0.0f;
	switch (stick)
	{
	case PadStick::PAD_L_X:
		value = state.Gamepad.sThumbLX;
		if (value > 0)
			ret = value / STICK_MAX;
		else if (value < 0)
			ret = value / -STICK_MIN;
		break;
	case PadStick::PAD_L_Y:
		value = state.Gamepad.sThumbLY;
		if (value > 0)
			ret = value / STICK_MAX;
		else if (value < 0)
			ret = value / -STICK_MIN;
		break;
	case PadStick::PAD_R_X:
		value = state.Gamepad.sThumbRX;
		if (value > 0)
			ret = value / STICK_MAX;
		else if (value < 0)
			ret = value / -STICK_MIN;
		break;
	case PadStick::PAD_R_Y:
		value = state.Gamepad.sThumbRY;
		if (value > 0)
			ret = value / STICK_MAX;
		else if (value < 0)
			ret = value / -STICK_MIN;
		break;
	case PadStick::PAD_L_TRIGGER:
		ret = state.Gamepad.bLeftTrigger / TRIGGER_MAX;
		break;
	case PadStick::PAD_R_TRIGGER:
		ret = state.Gamepad.bRightTrigger / TRIGGER_MAX;
		break;
	default:
		break;
	}

	return ret;
}

XBoxOutput::XBoxOutput()
{
	vibration = XINPUT_VIBRATION();
	lFrame = 0;
	rFrame = 0;
}

XBoxOutput::~XBoxOutput()
{

}

void XBoxOutput::Update()
{
	if (lFrame > 0)
	{
		lFrame--;
	}
	else
	{
		vibration.wLeftMotorSpeed = 0;
	}
	if (rFrame > 0)
	{
		rFrame--;
	}
	else
	{
		vibration.wRightMotorSpeed = 0;
	}


	XInputSetState(0, &vibration);
}

void XBoxOutput::SetVibration(int frame, float value)
{
	SetVibrationLeft(frame, value);
	SetVibrationRight(frame, value);
}

void XBoxOutput::SetVibrationLeft(int frame, float value)
{
	lFrame = frame;

	int v = int(value * VIBRATION_MAX);

	vibration.wLeftMotorSpeed = v;
}

void XBoxOutput::SetVibrationRight(int frame, float value)
{
	rFrame = frame;

	int v = int(value * VIBRATION_MAX);

	vibration.wRightMotorSpeed = v;
}

void XBoxOutput::StopVibration()
{
	StopVibrationLeft();
	StopVibrationRight();
}

void XBoxOutput::StopVibrationLeft()
{
	lFrame = 0;

	vibration.wLeftMotorSpeed = 0;
}

void XBoxOutput::StopVibrationRight()
{
	rFrame = 0;

	vibration.wRightMotorSpeed = 0;
}

SInput* SInput::instance = nullptr;

SInput::SInput()
{

}

SInput::~SInput()
{

}

void SInput::Init()
{
	instance = new SInput();
}

void SInput::UnInit()
{
	delete instance;
	instance = nullptr;
}

SInput& SInput::Instance()
{
	return *instance;
}

void SInput::Update()
{
	KeyBoard::Update();
	XBoxInput::Update();
	XBoxOutput::Update();
	Mouse::Update();
}

void Mouse::Update()
{
	for (int i = 0; i < 2; i++) {
		oldState[i] = state[i];	//1フレーム前の情報を保存
		state[i] = keyState[i];	//今の状態を適応
	}
}

void Mouse::SetMouseDown(int idx)
{
	keyState[idx] = true;
}

void Mouse::SetMouseUp(int idx)
{
	keyState[idx] = false;
}

bool Mouse::GetMouseDown(int idx)const
{
	return state[idx] && !oldState[idx];
}

bool Mouse::GetMouse(int idx) const
{
	return state[idx];
}

bool Mouse::GetMouseUp(int idx)const
{
	return !state[idx] && oldState[idx];
}

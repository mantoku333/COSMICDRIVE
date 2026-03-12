#include "Config.h"
#include "BinaryFile.h"
#include "SInput.h"
#include <string>

namespace app::test {

	const std::string CONFIG_FILE_PATH = "Save/Save.bin";

	void LoadConfig()
	{
		sf::file::BinaryFile file(CONFIG_FILE_PATH);
		SaveData data;
		// サイズが不一致でも、読み込める分だけ読み込む等の挙動なら一部復旧できるが
        // 基本的に構造体が変わるとReadAllは失敗するか、ゴミが入る。
        // ここでは単純にReadAllして、成功したら反映する。
		if (file.ReadAll(&data)) {
			gKeyConfig = data.keyConfig;
            gGameConfig = data.gameConfig;
		}
	}

	void SaveConfig()
	{
		sf::file::BinaryFile file(CONFIG_FILE_PATH);
        SaveData data;
        data.keyConfig = gKeyConfig;
        data.gameConfig = gGameConfig;
		file.WriteAll(data);
	}

	std::wstring KeyToString(Key key)
	{
		if (key >= Key::KEY_A && key <= Key::KEY_Z) {
			wchar_t c = (wchar_t)key;
			return std::wstring(1, c);
		}
		if (key >= Key::KEY_0 && key <= Key::KEY_9) {
			wchar_t c = (wchar_t)key;
			return std::wstring(1, c);
		}

		switch (key)
		{
		case Key::SPACE: return L"SPACE";
		case Key::ESCAPE: return L"ESC";
		case Key::KEY_SHIFT: return L"SHIFT";
		case Key::KEY_CTRL: return L"CTRL";
		case Key::KEY_LEFT: return L"LEFT";
		case Key::KEY_UP: return L"UP";
		case Key::KEY_RIGHT: return L"RIGHT";
		case Key::KEY_DOWN: return L"DOWN";
		case Key::KEY_BACK: return L"BACK";
		default: return L"KEY";
		}
	}
}

#include "Shader.h"
#include "sf.h"
#include <sstream>

HRESULT sf::dx::shader::Shader::LoadCSO(std::string fileName)
{
	HRESULT hr = E_FAIL;

	// ファイルを読み込む
	FILE* fp;
	fopen_s(&fp, fileName.c_str(), "rb");
	if (!fp)
	{
		throw std::runtime_error("CSOファイルの読み込みに失敗しました:" + fileName);
		return hr;
	}

	// ファイルのサイズを調べる
	int fileSize = 0;
	fseek(fp, 0, SEEK_END);
	fileSize = ftell(fp);

	// メモリに読み込み
	fseek(fp, 0, SEEK_SET);
	char* pData = new char[fileSize];
	fread(pData, fileSize, 1, fp);
	fclose(fp);

	// シェーダー作成
	hr = Make(pData, fileSize);

	// 終了処理
	if (pData) { delete[] pData; }

	if (FAILED(hr))
	{
		throw std::runtime_error("シェーダーの作成に失敗しました:" + fileName);
		return hr;
	}

	sf::debug::Debug::LogSafety("シェーダーの作成に成功しました:" + fileName);

	return hr;
}

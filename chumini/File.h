#pragma once
#include <vector>
#include <string>
#include "TextFile.h"
#include "BinaryFile.h"

namespace sf
{
	namespace file
	{
		//wstring->string
		std::string wstring2string(const std::wstring& src);

		//string->wstring
		std::wstring string2wstring(const std::string& src);

		//フォルダ内のファイルパスを取得する関数
		std::vector<std::string> SearchFolder(std::string folder);

		//フォルダ内の全てのファイルパスを取得する関数
		std::vector<std::string> SearchFolderAll(std::string folder);

		//ファイルの拡張子を取得する関数
		std::string GetExtension(const std::string path);

		//このファイルがフォルダか判断する関数
		bool IsFolder(std::string path);

		//フォルダ作成関数
		bool CreateFolder(std::string path);

		//ファイル削除関数
		bool DeleteFile(std::string path);

		//フォルダ削除関数(ファイルがある場合、自動で削除されます)
		bool DeleteFolder(std::string path);

		std::vector<std::string> ReadFile(std::string path);

		void WriteFile(std::string path, std::vector<std::string> data);
	}
}
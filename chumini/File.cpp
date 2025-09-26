#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include "File.h"
#include <filesystem>

#include <locale>
#include <codecvt>
#include <iostream>

std::string sf::file::wstring2string(const std::wstring& src)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t> > converter;
	return converter.to_bytes(src);
}

std::wstring sf::file::string2wstring(const std::string& src)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(src);
}

std::vector<std::string> sf::file::SearchFolder(std::string folder)
{
	std::vector<std::string> pathes;
	if (!std::filesystem::exists(folder))
	{
		return pathes;
	}

	// ディレクトリ内のファイルとフォルダをイテレート
	for (const auto& entry : std::filesystem::directory_iterator(folder)) {
		pathes.push_back(wstring2string(entry.path().c_str()));
	}

	return pathes;
}

std::vector<std::string> sf::file::SearchFolderAll(std::string folder)
{
	std::vector<std::string> pathes, ret;

	pathes = SearchFolder(folder);

	for (const auto& i : pathes) {
		//フォルダなら
		if (IsFolder(i))
		{
			//再帰呼び出し
			std::vector<std::string> childPathes = SearchFolderAll(i);

			//配列確保
			ret.reserve(ret.size() + childPathes.size());

			//retの後ろににchildPathesの要素をコピー
			std::copy(childPathes.begin(), childPathes.end(), std::back_inserter(ret));
		}
		else
		{
			//拡張子があれば戻り値に挿入
			ret.push_back(i);
		}
	}

	return ret;
}

std::string sf::file::GetExtension(const std::string path)
{
	//'.'の位置を後ろから検索
	size_t dotPos = path.rfind('.');

	//'.' が見つからない場合は空の文字列を返す
	if (dotPos == std::string::npos) {
		return "";
	}

	// ファイル名の最後にある場合は空の文字列を返す
	if (dotPos == path.length() - 1)
	{
		return "";
	}

	int e1 = path.rfind('\\');
	int e2 = path.rfind('/');
	int e = e1;
	if (e1 < e2)
	{
		e = e2;
	}

	//相対パス(./)(../)の'.'が選ばれた場合->'.'の後に'\\'は来れば
	if (dotPos < e)
	{
		return "";
	}

	//拡張子を返す（'.' の次の文字から最後まで）
	return path.substr(dotPos + 1);
}

bool sf::file::IsFolder(std::string path)
{
	return GetExtension(path).empty();
}

bool sf::file::CreateFolder(std::string path)
{
	return std::filesystem::create_directory(path);
}

bool sf::file::DeleteFile(std::string path)
{
	//このファイルがあるか確認
	if (std::filesystem::exists(path)) {

		try {
			std::filesystem::remove(path);
			return true;
		}
		catch (const std::exception& e) {
			return false;
		}

		return std::filesystem::remove(path);
	}
	else
	{
		return false;
	}
}

bool sf::file::DeleteFolder(std::string path)
{
	std::vector<std::string> pathes = file::SearchFolder(path);
	for (const auto& i : pathes) {

		//拡張子があれば
		if (!IsFolder(i))
		{
			//ファイル削除
			if (!DeleteFile(i))
			{
				return false;
			}
		}
		else
		{
			//フォルダ内のファイルを削除
			if (DeleteFolder(i))
			{
				return false;
			}

			//フォルダ本体を削除
			if (DeleteFile(i))
			{
				return false;
			}
		}
	}

	return DeleteFile(path);
}

std::vector<std::string> sf::file::ReadFile(std::string path)
{
	std::vector<std::string> data{};
	FILE* fp = NULL;
	fopen_s(&fp, path.c_str(), "r");

	if (!fp)
	{
		return data;
	}

	while (true)
	{
		char c[256]{};

		fgets(c, 256, fp);

		std::string str = c;

		//データが無くなれば
		if (str.empty())
		{
			break;
		}

		//改行コードを無くす
		if (str.back() == '\n')
		{
			str.erase(str.end() - 1);
		}
		data.push_back(str);
	}

	fclose(fp);

	return data;
}

void sf::file::WriteFile(std::string path, std::vector<std::string> data)
{

}

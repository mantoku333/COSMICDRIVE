#include "TextFile.h"
#include "File.h"

sf::file::TextFile::TextFile(std::string _fileName) :FileBase(_fileName)
{
}

sf::file::TextFile::~TextFile()
{
	if (fp)
	{
		fclose(fp);
		fp = nullptr;
	}
}

void sf::file::TextFile::Clean()
{
	if (fp)
	{
		fclose(fp);
		fp = nullptr;
	}

	fopen_s(&fp, fileName.c_str(), "w");
	fclose(fp);
	fp = nullptr;
}

std::string sf::file::TextFile::Read()
{
	std::string data;

	if (!fp)
	{
		fopen_s(&fp, fileName.c_str(), "r");
	}
	if (!fp)
	{
		return data;
	}


	char c[256]{};

	fgets(c, 256, fp);

	data = c;

	//改行コードを無くす
	if (data.back() == '\n')
	{
		data.erase(data.end() - 1);
	}
	return data;
}

std::vector<std::string> sf::file::TextFile::ReadAll()
{
	std::vector<std::string> data{};

	if (fp)
	{
		fclose(fp);
		fp = nullptr;
	}
	fopen_s(&fp, fileName.c_str(), "r");

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
		if (str.front() == EOF)
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
	fp = nullptr;

	return data;
}

bool sf::file::TextFile::WriteAll(std::vector<std::string> data)
{
	if (fp)
	{
		fclose(fp);
		fp = nullptr;
	}
	fopen_s(&fp, fileName.c_str(), "w");

	if (!fp)
	{
		return false;
	}

	for (auto& i : data) {
		std::string d = i;
		d.push_back('\n');
		const char* a = d.c_str();

		fwrite(a, sizeof(char), d.size(), fp);
	}

	fclose(fp);
	fp = nullptr;

	return true;
}

bool sf::file::TextFile::Write(std::vector<std::string> data)
{
	if (!fp)
	{
		fopen_s(&fp, fileName.c_str(), "a");
	}

	if (!fp)
	{
		return false;
	}

	for (auto& i : data) {
		std::string d = i;
		d.push_back('\n');
		const char* a = d.c_str();

		fwrite(a, sizeof(char), d.size(), fp);
	}

	return true;
}

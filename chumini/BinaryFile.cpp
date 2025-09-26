#include "BinaryFile.h"

sf::file::BinaryFile::BinaryFile(std::string _fileName) :FileBase(_fileName)
{
}

sf::file::BinaryFile::~BinaryFile()
{
	if (fp)
	{
		fclose(fp);
		fp = nullptr;
	}
}

void sf::file::BinaryFile::Clean()
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

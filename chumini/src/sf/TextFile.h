#pragma once
#include "FileBase.h"
#include <vector>

namespace sf
{
	namespace file
	{
		//テキストファイルにアクセスするクラス
		class TextFile :private FileBase
		{
		public:
			TextFile(std::string _fileName);
			~TextFile();

			void Clean();
			std::string Read();
			std::vector<std::string> ReadAll();
			bool WriteAll(std::vector<std::string> data);

			bool Write(std::vector<std::string> data);

		private:
			FILE* fp = nullptr;
		};
	}
}

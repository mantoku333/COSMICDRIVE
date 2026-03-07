#pragma once
#include <string>
namespace sf
{
	namespace file
	{
		class FileBase
		{
		public:
			FileBase(std::string _fileName) :fileName(_fileName){}

		protected:
			const std::string fileName;
		};
	}
}
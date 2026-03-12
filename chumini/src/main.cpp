#include "App.h"

int main(void)
{
#ifdef _DEBUG
	//メモリリーク検出
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	{
		app::Application application;
		application.Run();
	}

	return 0;
}

#include "App.h"

int main(void)
{
#ifdef _DEBUG
	//ƒƒ‚ƒŠƒŠ[ƒNŒŸo
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	{
		app::Application application;
		application.Run();
	}

	return 0;
}

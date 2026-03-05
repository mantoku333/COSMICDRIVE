#pragma once
#include <DirectXMath.h>
#include <d3d11.h>
#include <stdexcept>	//runtime_error‚ðŽg‚¤‚½‚ß

#pragma comment(lib,"d3d11.lib")

#define SAFE_RELEASE(p)	if(p!=nullptr){p->Release();p=nullptr;}
#define STR(p) #p

namespace sf
{
	namespace dx
	{
		//D3DŠî’êƒNƒ‰ƒX
		class D3D
		{
		public:
			virtual void Release() = 0;
		};
	}
}

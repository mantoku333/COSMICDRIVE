#pragma once
#include <DirectXMath.h>
#include <d3d11.h>
#include <stdexcept>	//runtime_errorを使うため

#pragma comment(lib,"d3d11.lib")

#define SAFE_RELEASE(p)	if(p!=nullptr){p->Release();p=nullptr;}
#define STR(p) #p

namespace sf
{
	namespace dx
	{
		//D3D基底クラス
		class D3D
		{
		public:
			virtual void Release() = 0;
		};
	}
}

#include "SwapChain.h"

sf::dx::SwapChain::~SwapChain()
{
	Release();
}

void sf::dx::SwapChain::Release()
{
	SAFE_RELEASE(d3dSwapchain);
}

void sf::dx::SwapChain::Flip()
{
	d3dSwapchain->Present(0, 0);
}

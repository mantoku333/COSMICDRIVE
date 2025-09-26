#include "RTV.h"

sf::dx::RTV::~RTV()
{
	Release();
}

void sf::dx::RTV::Release()
{
	SAFE_RELEASE(rtv);
}

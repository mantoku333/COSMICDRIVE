#include "Ref.h"

sf::ref::regacy::Ref::Ref() :ref(reinterpret_cast<uintptr_t>(this)) {}
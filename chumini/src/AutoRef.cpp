#include "AutoRef.h"

sf::ref::IautoRef::IautoRef() :ref(reinterpret_cast<uintptr_t>(this)) {}
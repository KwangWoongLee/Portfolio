#pragma once
#include "CorePch.h"

using CharacterId = StrongId<struct CharacterIdTag, int64_t>;

inline CharacterId constexpr INVALID_CHARACTER_ID{ 0 };

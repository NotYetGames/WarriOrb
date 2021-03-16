// Copyright (c) Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#pragma push_macro("ARRAY_COUNT")
#undef ARRAY_COUNT

#if STEAMSDK_FOUND == 0
#error Steam SDK not located.  Expected to be found in Engine/Source/ThirdParty/Steamworks/{SteamVersion}
#endif

THIRD_PARTY_INCLUDES_START

#include "steam/steam_api.h"

THIRD_PARTY_INCLUDES_END

#pragma pop_macro("ARRAY_COUNT")

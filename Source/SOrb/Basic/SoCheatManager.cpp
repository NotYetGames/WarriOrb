// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoCheatManager.h"
#include "Basic/SoGameInstance.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoCheatManager::AllowCheats() const
{
	if (const auto* GameInstance = USoGameInstance::GetInstance(this))
		return GameInstance->AllowCheats();

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoCheatManager::IsAllowCheatsPasswordValid(const FString& UserPassword)
{
#ifndef WARRIORB_PASSWORD_ENABLE_CONSOLE_CHEATS
#error "WARRIORB_PASSWORD_ENABLE_CONSOLE_CHEATS is not set in the SOrb.Build.cs file"
#endif // !WARRIORB_BUILD_VERSION
	static const FString CheatsPassword(TEXT(WARRIORB_PASSWORD_ENABLE_CONSOLE_CHEATS));
	return CheatsPassword.Equals(UserPassword, ESearchCase::CaseSensitive);
}

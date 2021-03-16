// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "INYSteamSubsystem.h"
#include "OnlineSubsystem.h"

const FName INYSteamSubsystem::SubsystemName{TEXT("Steam")};

bool INYSteamSubsystem::IsEnabledByConfigOrCLI()
{
	return IOnlineSubsystem::IsEnabled(SubsystemName);
}

IOnlineSubsystem* INYSteamSubsystem::GetUnrealSteamSubsystem()
{
	return IOnlineSubsystem::Get(SubsystemName);
}

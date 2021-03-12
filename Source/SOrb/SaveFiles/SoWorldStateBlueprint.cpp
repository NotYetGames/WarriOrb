// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoWorldStateBlueprint.h"
#include "Levels/SoLevelHelper.h"
#include "GameFramework/Actor.h"

#include "SoWorldState.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Blueprint interface
bool USoWorldState::ReadIntValue(const AActor* Actor, FName ValueName, UPARAM(Ref)int32& Value)
{
	if (!IsValid(Actor))
	{
		UE_LOG(LogSoWorldState, Error, TEXT("ReadIntValue failed: input actor is not valid (nullptr OR IsPendingKill)"));
		return false;
	}

	const FName LevelName = USoLevelHelper::GetLevelNameFromActor(Actor);
	const FName ActorName = Actor->GetFName();
	return FSoWorldState::Get().ReadIntValue(LevelName, ActorName, ValueName, Value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoWorldState::GetIntValue(const UObject* WorldContextObject, FName ValueName)
{
	const AActor* Actor = Cast<const AActor>(WorldContextObject);
	if (!IsValid(Actor))
	{
		UE_LOG(LogSoWorldState, Error, TEXT("GetIntValue failed: input actor is not valid (nullptr OR IsPendingKill)"));
		return false;
	}

	const FName LevelName = USoLevelHelper::GetLevelNameFromActor(Actor);
	const FName ActorName = Actor->GetFName();
	int32 TempValue = 0;
	FSoWorldState::Get().ReadIntValue(LevelName, ActorName, ValueName, TempValue);
	return TempValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoWorldState::ReadBoolValue(const AActor* Actor, FName ValueName, UPARAM(Ref)bool& bValue)
{
	if (!IsValid(Actor))
	{
		UE_LOG(LogSoWorldState, Error, TEXT("ReadBoolValue failed: input actor is not valid (nullptr OR IsPendingKill)"));
		return false;
	}

	const FName LevelName = USoLevelHelper::GetLevelNameFromActor(Actor);
	const FName ActorName = Actor->GetFName();
	return FSoWorldState::Get().ReadBoolValue(LevelName, ActorName, ValueName, bValue);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoWorldState::GetBoolValue(const UObject* WorldContextObject, FName ValueName)
{
	const AActor* Actor = Cast<const AActor>(WorldContextObject);
	if (!IsValid(Actor))
	{
		UE_LOG(LogSoWorldState, Error, TEXT("GetBoolValue failed: input actor is not valid (nullptr OR IsPendingKill)"));
		return false;
	}

	const FName LevelName = USoLevelHelper::GetLevelNameFromActor(Actor);
	const FName ActorName = Actor->GetFName();
	bool bValue = false;
	FSoWorldState::Get().ReadBoolValue(LevelName, ActorName, ValueName, bValue);
	return bValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoWorldState::WriteIntValue(const AActor* Actor, FName ValueName, int32 Value)
{
	if (!IsValid(Actor))
	{
		UE_LOG(LogSoWorldState, Error, TEXT("WriteIntValue failed: input actor is not valid (nullptr OR IsPendingKill)"));
		return;
	}

	const FName LevelName = USoLevelHelper::GetLevelNameFromActor(Actor);
	const FName ActorName = Actor->GetFName();
	FSoWorldState::Get().WriteIntValue(LevelName, ActorName, ValueName, Value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoWorldState::WriteBoolValue(const AActor* Actor, FName ValueName, bool bValue)
{
	if (!IsValid(Actor))
	{
		UE_LOG(LogSoWorldState, Error, TEXT("WriteBoolValue failed: input actor is not valid (nullptr OR IsPendingKill)"));
		return;
	}

	const FName LevelName = USoLevelHelper::GetLevelNameFromActor(Actor);
	const FName ActorName = Actor->GetFName();
	FSoWorldState::Get().WriteBoolValue(LevelName, ActorName, ValueName, bValue);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoWorldState::ReadFloatValue(const AActor* Actor, FName ValueName, UPARAM(Ref)float& Value)
{
	if (!IsValid(Actor))
	{
		UE_LOG(LogSoWorldState, Error, TEXT("ReadFloatValue failed: input actor is not valid (nullptr OR IsPendingKill)"));
		return false;
	}

	const FName LevelName = USoLevelHelper::GetLevelNameFromActor(Actor);
	const FName ActorName = Actor->GetFName();
	return FSoWorldState::Get().ReadFloatValue(LevelName, ActorName, ValueName, Value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoWorldState::WriteFloatValue(const AActor* Actor, FName ValueName, float Value)
{
	if (!IsValid(Actor))
	{
		UE_LOG(LogSoWorldState, Error, TEXT("WriteFloatValue failed: input actor is not valid (nullptr OR IsPendingKill)"));
		return;
	}

	const FName LevelName = USoLevelHelper::GetLevelNameFromActor(Actor);
	const FName ActorName = Actor->GetFName();
	FSoWorldState::Get().WriteFloatValue(LevelName, ActorName, ValueName, Value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoWorldState::ReadVectorValue(const AActor* Actor, FName ValueName, UPARAM(Ref)FVector& Value)
{
	if (!IsValid(Actor))
	{
		UE_LOG(LogSoWorldState, Error, TEXT("ReadVectorValue failed: input actor is not valid (nullptr OR IsPendingKill)"));
		return false;
	}

	const FName LevelName = USoLevelHelper::GetLevelNameFromActor(Actor);
	const FName ActorName = Actor->GetFName();
	if (!CheckAndValidateFName(ActorName))
	{
		UE_LOG(LogSoWorldState, Error, TEXT("ReadVectorValue failed: ActorName for input Actor is not valid :O"));
		return false;
	}

	return FSoWorldState::Get().ReadVectorValue(LevelName, ActorName, ValueName, Value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoWorldState::WriteVectorValue(const AActor* Actor, FName ValueName, const FVector& Value)
{
	if (!IsValid(Actor))
	{
		UE_LOG(LogSoWorldState, Error, TEXT("WriteVectorValue failed: input actor is not valid (nullptr OR IsPendingKill)"));
		return;
	}

	const FName LevelName = USoLevelHelper::GetLevelNameFromActor(Actor);
	const FName ActorName = Actor->GetFName();
	if (!CheckAndValidateFName(ActorName))
	{
		UE_LOG(LogSoWorldState, Error, TEXT("WriteVectorValue failed: ActorName for input Actor is not valid :O"));
		return;
	}

	FSoWorldState::Get().WriteVectorValue(LevelName, ActorName, ValueName, Value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoWorldState::AddNameToSet(const AActor* Actor, FName Name)
{
	if (!IsValid(Actor))
	{
		UE_LOG(LogSoWorldState, Error, TEXT("AddNameToSet failed: input actor is not valid (nullptr OR IsPendingKill)"));
	}

	const FName LevelName = USoLevelHelper::GetLevelNameFromActor(Actor);
	if (!CheckAndValidateFName(Name))
	{
		UE_LOG(LogSoWorldState, Error, TEXT("AddNameToSet failed: Name for input Actor = `%s` is not valid :O"), *Actor->GetName());
		return;
	}

	FSoWorldState::Get().AddNameToSet(LevelName, Name);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoWorldState::AddActorNameToSet(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		UE_LOG(LogSoWorldState, Error, TEXT("AddActorNameToSet failed: input actor is not valid (nullptr OR IsPendingKill)"));
	}

	const FName LevelName = USoLevelHelper::GetLevelNameFromActor(Actor);
	const FName ActorName = Actor->GetFName();
	if (!CheckAndValidateFName(ActorName))
	{
		UE_LOG(LogSoWorldState, Error, TEXT("AddActorNameToSet failed: ActorName for input Actor is not valid :O"));
		return;
	}

	FSoWorldState::Get().AddNameToSet(LevelName, ActorName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoWorldState::RemoveActorNameFromSet(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		UE_LOG(LogSoWorldState, Error, TEXT("RemoveActorNameFromSet failed: input actor is not valid (nullptr OR IsPendingKill)"));
		return;
	}

	const FName LevelName = USoLevelHelper::GetLevelNameFromActor(Actor);
	const FName ActorName = Actor->GetFName();
	if (!CheckAndValidateFName(ActorName))
	{
		UE_LOG(LogSoWorldState, Error, TEXT("RemoveActorNameFromSet failed: ActorName for input Actor is not valid :O"));
		return;
	}

	FSoWorldState::Get().RemoveNameFromSet(LevelName, ActorName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoWorldState::AddMyNameToSet(const UObject* WorldContextObject)
{
	AddActorNameToSet(Cast<AActor>(WorldContextObject));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoWorldState::RemoveMyNameFromSet(const UObject* WorldContextObject)
{
	RemoveActorNameFromSet(Cast<AActor>(WorldContextObject));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoWorldState::IsMyNameInSet(const UObject* WorldContextObject)
{
	return IsActorNameInSet(Cast<AActor>(WorldContextObject));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoWorldState::RemoveNameFromSet(const AActor* Actor, FName Name)
{
	if (!IsValid(Actor))
	{
		UE_LOG(LogSoWorldState, Error, TEXT("RemoveNameFromSet failed: input actor is not valid (nullptr OR IsPendingKill)"));
		return;
	}

	const FName LevelName = USoLevelHelper::GetLevelNameFromActor(Actor);
	FSoWorldState::Get().RemoveNameFromSet(LevelName, Name);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoWorldState::IsNameInSet(const AActor* Actor, FName Name)
{
	if (!IsValid(Actor))
	{
		UE_LOG(LogSoWorldState, Error, TEXT("IsNameInSet failed: input actor is not valid (nullptr OR IsPendingKill)"));
		return false;
	}

	const FName LevelName = USoLevelHelper::GetLevelNameFromActor(Actor);
	return FSoWorldState::Get().IsNameInSet(LevelName, Name);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoWorldState::IsActorNameInSet(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		UE_LOG(LogSoWorldState, Error, TEXT("IsActorNameInSet failed: input actor is not valid (nullptr OR IsPendingKill)"));
		return false;
	}

	const FName LevelName = USoLevelHelper::GetLevelNameFromActor(Actor);
	const FName ActorName = Actor->GetFName();
	if (!CheckAndValidateFName(ActorName))
	{
		UE_LOG(LogSoWorldState, Error, TEXT("IsActorNameInSet failed: ActorName for input Actor is not valid :O"));
		return false;
	}

	return FSoWorldState::Get().IsNameInSet(LevelName, ActorName);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoWorldState::CheckAndValidateFName(FName Name)
{
	if (Name.IsNone())
	{
		UE_LOG(LogSoWorldState, Warning, TEXT("CheckAndValidateFName: Got Name = `None`."));
		return false;
	}

	if (!Name.IsValidIndexFast())
	{
		UE_LOG(LogSoWorldState, Warning, TEXT("Got Name.IsValidIndexFast() == false"));
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoWorldState::SetDisplayedProgressName(FName DisplayedProgressName)
{
	if (DisplayedProgressName != NAME_None)
		FSoWorldState::Get().SetDisplayedProgressName(DisplayedProgressName);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoDifficulty USoWorldState::GetGameDifficulty()
{
	return FSoWorldState::Get().GetGameDifficulty();
}

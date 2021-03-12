// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEffectBase.h"
#include "GameFramework/Actor.h"

#include "CharacterBase/SoMortal.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/Helpers/SoStringHelper.h"
#include "Online/Achievements/SoAchievementManager.h"

DEFINE_LOG_CATEGORY(LogSoEffectSystem);

FString USoEffectBase::TextNamespace(TEXT("EffectInstance"));


#if WITH_EDITOR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEffectBase::PreSave(const ITargetPlatform* TargetPlatform)
{
	UpdateLocalizedFields();
	Super::PreSave(TargetPlatform);
}
#endif // WITH_EDITOR

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEffectBase::UpdateLocalizedFields()
{
	const FString ObjectName = USoStringHelper::GetObjectBaseName(this);
	DisplayName = FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText(
		*DisplayName.ToString(),
		*TextNamespace,
		*(ObjectName + "_name")
	);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEffectBase::CanBeApplied_Implementation(AActor* TargetOwner)
{
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEffectBase::Apply(AActor* InOwner)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Apply"), STAT_Apply, STATGROUP_SoEffect);

#if WITH_EDITOR
	if (InOwner == nullptr)
	{
		UE_LOG(LogSoEffectSystem, Error, TEXT("USoEffectBase::Apply called on nullptr!"));
		return false;
	}

	if (!InOwner->GetClass()->ImplementsInterface(USoMortal::StaticClass()))
	{
		UE_LOG(LogSoEffectSystem, Error, TEXT("USoEffectBase::Apply called on %s but it does not implement the appropriate interface!"), *InOwner->GetName());
		return false;
	}
#endif

	Owner = InOwner;

	Counter = 0.0f;
	CountNum = 0;

	OnApplied();

	return EffectType != ESoEffectType::EET_Instant;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEffectBase::Reapply_Implementation()
{
	switch (EffectType)
	{
		case ESoEffectType::EET_TimeBased:
			Counter = 0.0f;
			return true;

		case ESoEffectType::EET_Counted:
			CountNum = 0;
			return true;

		default:
			return false;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEffectBase::Tick(float DeltaSeconds)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Tick"), STAT_Tick, STATGROUP_SoEffect);

	const float RealTick = EffectType == ESoEffectType::EET_TimeBased ? FMath::Min(DeltaSeconds, Duration - Counter) : DeltaSeconds;
	Counter += DeltaSeconds;

	if (bKidWantsOnTick)
		OnTick(RealTick);

	if (EffectType == ESoEffectType::EET_TimeBased && Counter > Duration)
	{
		OnDurationOver();
		return false;
	}

	if (EffectType == ESoEffectType::EET_Counted && CountNum >= MaxCount)
	{
		OnDurationOver();
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEffectBase::IncreaseCount()
{
	CountNum += 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEffectBase::PrintMessage(const FString& TextToPrint)
{
	UE_LOG(LogSoEffectSystem, Error, TEXT("%s"), *TextToPrint);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEffectBase::PrintError(const FString& TextToPrint, bool bJustWarning)
{
	if (bJustWarning)
		UE_LOG(LogSoEffectSystem, Warning, TEXT("%s"), *TextToPrint)
	else
		UE_LOG(LogSoEffectSystem, Error, TEXT("%s"), *TextToPrint);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoCharacter* USoEffectBase::GetSoChar()
{
	return USoStaticHelper::GetPlayerCharacterAsSoCharacter(Owner);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEffectBase::UnlockAchievement(AActor* WorldContext, FName Name)
{
	if (WorldContext != nullptr)
		USoAchievementManager::Get(WorldContext).UnlockAchievement(WorldContext, Name);
	else
	{
		if (Owner != nullptr)
			USoAchievementManager::Get(Owner).UnlockAchievement(Owner, Name);
		else
			UE_LOG(LogSoEffectSystem, Warning, TEXT("Failed to unlock achievement: %s in USoEffectBase::UnlockAchievement, no valid world context!"), *Name.ToString())
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEffectBase::GetPercent()
{
	if (EffectType == ESoEffectType::EET_Counted)
		return FMath::Clamp(CountNum / static_cast<float>(MaxCount), 0.0f, 1.0f);

	return FMath::Clamp(Counter / Duration, 0.0f, 1.0f);
}

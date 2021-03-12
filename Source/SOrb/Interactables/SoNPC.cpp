// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoNPC.h"

#include "Basic/Helpers/SoStaticHelper.h"
#include "SaveFiles/SoWorldStateBlueprint.h"
#include "Character/SoCharacter.h"

const FName NameKnownName = FName("NameKnown");
const FName AlreadyTriggered = FName("AlreadyTriggered");

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoNPC::ASoNPC(const FObjectInitializer& ObjectInitializer) :
	Super()
{
	DialogueData.PositionIndex = 0;
	DialogueData.FadeInAnimName = FName("NPC_Arrive");
	DialogueData.FadeOutAnimName = FName("NPC_Leave");
	DialogueData.FadeOutAnimName = FName("NPCInactive_Leave");
	DialogueData.TextBoxY = -876.0f;
	DialogueData.FloorPosition = FVector2D(-1048.0f, -876.0f);

	bSecondKeyPreferred = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoNPC::BeginPlay()
{
	Super::BeginPlay();

	if (bHandlePostLoad || bSaveIfNameKnown || bTriggerOnlyOnce)
		USoEventHandlerHelper::SubscribeToSoPostLoad(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoNPC::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (bHandlePostLoad || bSaveIfNameKnown || bTriggerOnlyOnce)
		USoEventHandlerHelper::UnsubscribeFromSoPostLoad(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoNPC::HandleSoPostLoad_Implementation()
{

	if (bSaveIfNameKnown)
	{
		bool bNameKnown = false;
		USoWorldState::ReadBoolValue(this, NameKnownName, bNameKnown);
		DialogueData.ParticipantDisplayName = bNameKnown ? DlgParticipantDisplayName : DlgDisplayNameIfUnkown;
	}

	if (bTriggerOnlyOnce)
	{
		bool bTriggered = false;
		USoWorldState::ReadBoolValue(this, AlreadyTriggered, bTriggered);
		if (bTriggered || bActiveByDefault == USoWorldState::IsMyNameInSet(this))
			Deactivate();
		else
			Activate();
	}

	OnGameReloadBP();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoNPC::ShouldSpawnVO_Implementation(ESoVoiceType VoiceType) const
{
	if (ASoCharacter* Player = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
	{
		if ((GetActorLocation() - Player->GetActorLocation()).SizeSquared() > MaxVODistance * MaxVODistance)
			return false;

		if (Player->IsInDialogue())
			return false;
	}

	if (VoiceType == ESoVoiceType::SoVoiceTypeNoise)
		return true;

	return FMath::RandRange(0.0f, 1.0f) <= VOChance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoNPC::IsAlreadyTriggered()
{
	bool bTriggered = false;
	USoWorldState::ReadBoolValue(this, AlreadyTriggered, bTriggered);
	return bTriggerOnlyOnce && bTriggered;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoNPC::Interact_Implementation(ASoCharacter* Character)
{
	OnInteract(Character);

	if (bTriggerOnlyOnce)
	{
		USoWorldState::WriteBoolValue(this, AlreadyTriggered, true);
		Deactivate();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoNPC::ModifyBoolValue_Implementation(FName ValueName, bool bValue)
{
	if (bSaveIfNameKnown && ValueName == NameKnownName)
	{
		USoWorldState::WriteBoolValue(this, NameKnownName, bValue);
		DialogueData.ParticipantDisplayName = bValue ? DlgParticipantDisplayName : DlgDisplayNameIfUnkown;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoNPC::GetBoolValue_Implementation(FName ValueName) const
{
	if (bSaveIfNameKnown && ValueName == NameKnownName)
	{
		bool bNameKnown = false;
		USoWorldState::ReadBoolValue(this, NameKnownName, bNameKnown);
		return bNameKnown;
	}

	return false;
}

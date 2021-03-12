// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUIStationPanel.h"

#include "Components/RetainerBox.h"
#include "Components/Image.h"

#include "SplineLogic/SoMarker.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Character/SoCharacter.h"
#include "Character/SoCharStates/SoATeleport.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIStationPanel::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Collapsed);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIStationPanel::SetInGameActivityEnabled_Implementation(UObject* Source, bool bEnable)
{
	SetVisibility(bEnable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (bEnable)
	{
		TargetStation = nullptr;
		OnUIStateActivated();
		ActiveState = ESoStationState::ESS_FadeIn;
		PlayFadeAnim(false);
	}
	else
	{
		if (ActiveState != ESoStationState::ESS_Closed)
		{
			SetVisibility(ESlateVisibility::Collapsed);
			ActiveState = ESoStationState::ESS_Closed;
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIStationPanel::Update_Implementation(float DeltaSeconds)
{
	switch (ActiveState)
	{
		case ESoStationState::ESS_FadeOut:
			if (!IsAnyAnimationPlaying())
			{
				SetVisibility(ESlateVisibility::Collapsed);
				ActiveState = ESoStationState::ESS_Closed;

				if (TargetStation != nullptr)
				{
					USoStaticHelper::GetPlayerCharacterAsSoCharacter(this)->SoATeleport->SetResetCameraViewAfterPortal();
					USoStaticHelper::GetPlayerCharacterAsSoCharacter(this)->SoATeleport->SetupTeleport(TargetStation->GetSplineLocation(),
																									   TargetStation->GetActorLocation().Z,
																									   false,
																									   false);
					// true because state change happened already in teleport
					return true;
				}
				return false;
			}
			return true;

		case ESoStationState::ESS_FadeIn:
			if (!IsAnyAnimationPlaying())
			{
				ActiveState = ESoStationState::ESS_Openned;
			}
			return true;

		case ESoStationState::ESS_Closed:
			return false;

		case ESoStationState::ESS_Openned:
		default:
			return true;
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FText& USoUIStationPanel::GetTextFromStation(ESoStation Station)
{
#define LOCTEXT_NAMESPACE "StationNames"

	static FText StationNames[static_cast<int32>(ESoStation::ES_ThisSoStay) + 1] =
	{
		LOCTEXT("crossroadcave", "Crossroad Cave"),
		LOCTEXT("outpost", "Obedient Outpost"),
		LOCTEXT("lavatemple", "Remnant Road"),
		LOCTEXT("wrackedWaters", "Wracked Waters"),
		LOCTEXT("away", "World's End"),
		LOCTEXT("stay", "Stay"),
	};

#undef LOCTEXT_NAMESPACE

	if (Station <= ESoStation::ES_ThisSoStay && Station >= ESoStation::ES_CrossroadCave)
		return StationNames[static_cast<int32>(Station)];

	return FText::GetEmpty();
}

static const FName StationNameCrossroadCave = FName("CrossroadCave");
static const FName StationNameOutpost = FName("Outpost");
static const FName StationNameLavaTemple = FName("LavaTemple");
static const FName StationNameWrackedWaters = FName("WrackedWaters");

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FName USoUIStationPanel::GetNameFromStation(ESoStation Station)
{
	static FName StationNames[static_cast<int32>(ESoStation::ES_Away)] =
	{
		StationNameCrossroadCave,
		StationNameOutpost,
		StationNameLavaTemple,
		StationNameWrackedWaters
	};

	if (Station < ESoStation::ES_Away && Station >= ESoStation::ES_CrossroadCave)
		return StationNames[static_cast<int32>(Station)];

	return NAME_None;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoStation USoUIStationPanel::GetStationFromName(FName Name)
{
	if (Name == StationNameCrossroadCave)
		return ESoStation::ES_CrossroadCave;
	if (Name == StationNameOutpost)
		return ESoStation::ES_Outpost;
	if (Name == StationNameLavaTemple)
		return ESoStation::ES_Lava;
	if (Name == StationNameWrackedWaters)
		return ESoStation::ES_Water;

	return ESoStation::ES_ThisSoStay;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIStationPanel::SetActiveState(ESoStationState NewState)
{
	if (ActiveState != NewState)
	{
		ActiveState = NewState;
		if (ActiveState == ESoStationState::ESS_FadeOut)
			PlayFadeAnim(true);
	}
}

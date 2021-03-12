// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/InGame/SoUIGameActivity.h"

#include "SoUIStationPanel.generated.h"

class ASoMarker;
class USoUIButtonArray;

////////////////////////////////////////////////////////////////////////////////////////
UENUM(BlueprintType)
enum class ESoStation : uint8
{
	ES_CrossroadCave					UMETA(DisplayName = "CrossroadCave"),
	ES_Outpost							UMETA(DisplayName = "Outpost"),
	ES_Lava								UMETA(DisplayName = "LavaTemple"),
	ES_Water							UMETA(DisplayName = "WrackedWaters"),

	ES_Away								UMETA(DisplayName = "Away"),

	ES_ThisSoStay						UMETA(DisplayName = "ThisSoStay")
};

UENUM(BlueprintType)
enum class ESoStationState : uint8
{
	// only the buttons are visible
	ESS_FadeIn			UMETA(DisplayName = "FadeIn"),
	ESS_FadeOut			UMETA(DisplayName = "FadeOut"),

	ESS_Openned			UMETA(DisplayName = "Opened"),
	ESS_Closed			UMETA(DisplayName = "Closed")
};

/**
 *
 */
UCLASS()
class SORB_API USoUIStationPanel : public USoInGameUIActivity
{
	GENERATED_BODY()

	// Begin UUserWidget Interface
	void NativeConstruct() override;
	// End UUserWidget Interface


	// Begin USoInGameUIActivity
	bool SetInGameActivityEnabled_Implementation(UObject* Source, bool bEnable) override;
	bool Update_Implementation(float DeltaSeconds) override;
	bool ShouldHideUIElements_Implementation() const override { return true; }
	//End USoInGameUIActivity

public:
	UFUNCTION(BlueprintPure, Category = ">Station")
	static const FText& GetTextFromStation(ESoStation Station);

	UFUNCTION(BlueprintPure, Category = ">Station")
	static FName GetNameFromStation(ESoStation Station);

	UFUNCTION(BlueprintPure, Category = ">Station")
	static ESoStation GetStationFromName(FName Name);

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = ">Events")
	void OnUIStateActivated();

	UFUNCTION(BlueprintImplementableEvent, Category = ">Events")
	void PlayFadeAnim(bool bOut);

	UFUNCTION(BlueprintCallable, Category = ">Station")
	void SetActiveState(ESoStationState NewState);

protected:

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIButtonArray* ButtonArray;

	UPROPERTY(BlueprintReadOnly, Category = ">Station")
	ESoStationState ActiveState = ESoStationState::ESS_Closed;

	UPROPERTY(BlueprintReadWrite, Category = ">Station")
	ASoMarker* TargetStation = nullptr;
};

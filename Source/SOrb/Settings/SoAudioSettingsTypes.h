// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

#include "SoAudioSettingsTypes.generated.h"

class UFMODEvent;

UENUM(BlueprintType)
enum class ESoSFX : uint8
{
	None = 0,

	SettingsLineSwitch,
	SettingsValueSwitch,
	SettingsApply,
	SettingsRestore,
	SettingsPressed,
	SettingsClose,

	MenuButtonSwitch,
	MenuButtonPressed,
	MenuReturnToRoot
};

// All the voice entries
USTRUCT(BlueprintType)
struct FSoVoiceEntry
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, Category = Voice, meta = (DisplayName = "Event"))
	TSoftObjectPtr<UFMODEvent> EventPtr;

	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, Category = Voice)
	FText Text;
};

// All the voice entries
USTRUCT(BlueprintType)
struct FSoVoiceEntryRandomText
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, Category = Voice, meta = (DisplayName = "Event"))
	TSoftObjectPtr<UFMODEvent> EventPtr;

	UPROPERTY(BlueprintReadWrite, Config, EditAnywhere, Category = Voice)
	TArray<FText> Texts;
};


// The default values here represent the default audio settings for the game
// As defined by the CPP
USTRUCT()
struct SORB_API FSoAudioSettings
{
	GENERATED_USTRUCT_BODY()
	typedef FSoAudioSettings Self;

public:
	FSoAudioSettings() {}
	FSoAudioSettings(bool InMuteAudio, bool bInMuteAudioWhenUnfocused, bool bInMuteVoiceGibberish, bool bInMuteDialogueVoiceGibberish,
		float InVolumeMaster, float InVolumeMusic, float InVolumeSFX) :
		bMuteAudio(InMuteAudio), bMuteAudioWhenUnfocused(bInMuteAudioWhenUnfocused), bMuteVoiceGibberish(bInMuteVoiceGibberish),
		bMuteDialogueVoiceGibberish(bInMuteDialogueVoiceGibberish),
		VolumeMaster(InVolumeMaster), VolumeMusic(InVolumeMusic),  VolumeSFX(InVolumeSFX)
	{

	}

	bool operator==(const Self& Other) const
	{
		return Other.bMuteAudio == bMuteAudio &&
			Other.bMuteAudioWhenUnfocused == bMuteAudioWhenUnfocused &&
			Other.bMuteVoiceGibberish == bMuteVoiceGibberish &&
			Other.bMuteDialogueVoiceGibberish == bMuteDialogueVoiceGibberish &&
			FMath::IsNearlyEqual(VolumeMaster, Other.VolumeMaster, KINDA_SMALL_NUMBER) &&
			FMath::IsNearlyEqual(VolumeMusic, Other.VolumeMusic, KINDA_SMALL_NUMBER) &&
			FMath::IsNearlyEqual(VolumeSFX, Other.VolumeSFX, KINDA_SMALL_NUMBER);
	}

public:
	UPROPERTY()
	bool bMuteAudio = false;

	UPROPERTY()
	bool bMuteAudioWhenUnfocused = true;

	UPROPERTY()
	bool bMuteVoiceGibberish = false;;

	UPROPERTY()
	bool bMuteDialogueVoiceGibberish = false;;

	UPROPERTY()
	float VolumeMaster = 1.f;

	UPROPERTY()
	float VolumeMusic = 1.f;

	UPROPERTY()
	float VolumeSFX = 1.f;
};

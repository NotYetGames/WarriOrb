// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

#include "SoLevelTypes.generated.h"

class ASoMarker;
class ALevelSequenceActor;
class UAnimSequenceBase;

USTRUCT(BlueprintType, Blueprintable)
struct FSoLevelEnterParams
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere)
	UAnimSequenceBase* Animation;

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<ALevelSequenceActor> Sequence;

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<AActor> Camera;

	UPROPERTY(EditAnywhere)
	float HideCharacterTime = -1.0f;

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<ASoMarker> TeleportAfterCutscene;

	// UPROPERTY(BlueprintReadWrite, EditAnywhere)
	// UTexture2D* LoadingImage = nullptr;

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UTexture2D> LoadingImagePtr;


	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText LoadingDescription;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText DisplayTextOnBlackFadeDuringFirstMapStart;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FText> RandomLoadingScreenTexts;
};

USTRUCT(BlueprintType, Blueprintable)
struct FSoChapterMapParams
{
	GENERATED_USTRUCT_BODY()
public:
	FORCEINLINE bool IsValid() const { return Level.IsValid(); }

	// Aka the ChapterName
	FORCEINLINE FString GetMapNameString() const { return Level.GetAssetName(); }
	FORCEINLINE FName GetMapName() const { return FName(*GetMapNameString()); }

public:
	// Current Level
	UPROPERTY(EditAnywhere, meta = (AllowedClasses = "World"))
	FSoftObjectPath Level;

	// Next Level after the current level if any
	UPROPERTY(EditAnywhere, meta = (AllowedClasses = "World"))
	FSoftObjectPath NextLevel;

	// Friendly localizable name
	UPROPERTY(EditAnywhere)
	FText NameText;

	UPROPERTY(EditAnywhere)
	FSoLevelEnterParams LevelEnterParams;
};

USTRUCT(BlueprintType, Blueprintable)
struct FSoLevelWeatherVFX
{
	GENERATED_USTRUCT_BODY()

public:
	FORCEINLINE FString GetLevelNameString() const { return Level.GetAssetName(); }
	FORCEINLINE FName GetLevelName() const { return FName(*GetLevelNameString()); }

public:
	UPROPERTY(EditAnywhere, Config, meta = (AllowedClasses = "World"))
	FSoftObjectPath Level;

	UPROPERTY(EditAnywhere, Config, meta = (DisplayName = "Particles"))
	TSoftObjectPtr<UParticleSystem> ParticlesPtr;
};


USTRUCT()
struct FSoLevelParams
{
	GENERATED_USTRUCT_BODY()

public:
	FORCEINLINE FString GetLevelNameString() const { return Level.GetAssetName(); }
	FORCEINLINE FName GetLevelName() const { return FName(*GetLevelNameString()); }

public:
	UPROPERTY(EditAnywhere, Config, meta = (AllowedClasses = "World"))
	FSoftObjectPath Level;

	UPROPERTY(EditAnywhere, Config, meta = (AllowedClasses = "World"))
	FName SkyPresetName;
};

/**
*  Helper struct to setup dependent levels for a claimed level
*/
// USTRUCT(BlueprintType)
// struct FSoDependentLevelDesc
// {
// 	GENERATED_USTRUCT_BODY()
//
// public:
// 	/** Name of the claimed level */
// 	UPROPERTY(EditAnywhere, meta = (AllowedClasses = "World"))
// 	FSoftObjectPath Level;
//
// 	/** dependent levels for the claimed level */
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowedClasses = "World"))
// 	TArray<FSoftObjectPath> ClaimedLevels;
// };


/** fname array wrapper struct so it can be stored in map */
// USTRUCT()
// struct FSoDependentLevels
// {
// 	GENERATED_USTRUCT_BODY()
//
// public:
// 	UPROPERTY(VisibleAnywhere)
// 	TArray<FName> LevelNames;
// };

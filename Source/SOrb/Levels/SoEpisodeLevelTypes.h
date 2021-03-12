// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

#include "Items/SoItem.h"
#include "SoLevelTypes.h"

#include "SoEpisodeLevelTypes.generated.h"

class UTexture2D;

// UENUM(BlueprintType)
// enum class ESoEpisodeProgress : uint8
// {
// 	Started = 0,
// 	Completed
// };

// Data for each episode
USTRUCT(BlueprintType, Blueprintable)
struct FSoEpisodeMapParams
{
	GENERATED_USTRUCT_BODY()
public:
	FORCEINLINE bool IsValid() const { return Level.IsValid(); }

	// Aka the EpisodeName
	FORCEINLINE FString GetMapNameString() const { return Level.GetAssetName(); }
	FORCEINLINE FName GetMapName() const { return FName(*GetMapNameString()); }

public:
	// The persistent map level
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (AllowedClasses = "World"))
	FSoftObjectPath Level;

	// The checkpoint location in the persistent level
	// NAME_None means default/start location.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName CheckpointLocation = NAME_None;

	// Friendly display name
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText DisplayName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UTexture2D* Image = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText Description;

	// All the equipped items this char has when entering the Episode
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Character)
	TArray<FSoItem> EnterEquippedItems;

	// All the equipped spells this char has when entering the Episode
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Character)
	TArray<FSoItem> EnterEquippedSpells;

	// Items in the inventory the char has when entering the Episode
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Character)
	TArray<FSoItem> EnterInventoryItems;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MaxHealth = 30.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 SpellsCapacity = 2;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText CompletedSubtitle;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText CompletedDescription;

	// Means if we can save progress in this episode to a checkpoint or something like this
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bCanSave = false;

	// Useful if want to see if we can just remove the Teleport to SoulKeeper button
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bCanHaveSoulKeeper = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSoLevelEnterParams LevelEnterParams;
};

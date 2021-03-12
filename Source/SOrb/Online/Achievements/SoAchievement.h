// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

#include "SoAchievement.generated.h"

struct FOnlineAchievement;
struct FOnlineAchievementDesc;

// https://partner.steamgames.com/doc/features/achievements#7
UENUM(BlueprintType)
enum class ESoSteamStatType : uint8
{
	Int = 0,
	Float

	// TODO add AVG Rate
};

// Hold the stats for the player progress
USTRUCT(BlueprintType)
struct FSoAchievement
{
	GENERATED_USTRUCT_BODY()
	typedef FSoAchievement Self;

public:
	static const TSet<FName>& GetAllAchievementNames();
	static float GetUnlockedProgressValue() { return 1.f; }
	static FName SteamStatTypeToName(const ESoSteamStatType Type);

	// Valid ingame Achievement Name, this is the only valid one
	static bool IsValidName(const FName Name) { return GetAllAchievementNames().Contains(Name); }
	static bool IsValidName(const FString& NameStr) { return IsValidName(FName(*NameStr)); }

	FSoAchievement() {}

	void SetBySteam(bool bValue) { bSetBySteam = bValue; }
	bool IsSetBySteam() const { return bSetBySteam; }

	bool IsValid() const { return Name.IsValid() && !Name.IsNone(); }
	bool IsUnlocked() const { return FMath::IsNearlyEqual(Progress, GetMaxProgressAsFloat()); }
	bool IsProgressAchievement() const
	{
		return MaxProgress > 1 && SteamUserStatName != NAME_None;
	}

	// Getters/Setters
	FName GetName() const { return Name; }
	int32 GetProgress() const { return FMath::FloorToInt(GetProgressAsFloat()); }
	float GetProgressAsFloat() const { return Progress; }
	int32 GetMaxProgress() const { return MaxProgress; }
	float GetMaxProgressAsFloat() const { return static_cast<float>(GetMaxProgress()); }

	// Checks if this progress would make this unlocked
	bool WouldMakeItUnlocked(float FutureProgress) const;

	// Sets the progress for achievements that support progress
	void SetProgress(float InProgress);

	// Unlocks the achievement
	void Unlock();

	// Resets the achievement progress, should only be used in development
	void Reset() { SetProgress(0.f); }

	// Returns debugging string to print out achievement info
	FString ToString() const;

	bool operator==(const Self& Other)
	{
		return Name == Other.Name &&
			bIsHidden == Other.bIsHidden &&
			MaxProgress == Other.MaxProgress &&
			SteamUserStatName == Other.SteamUserStatName &&
			SteamUserStatType == Other.SteamUserStatType &&
			bIsOnSteam == Other.bIsOnSteam;
	}
	bool operator!=(const Self& Other)
	{
		return !(*this == Other);
	}

protected:
	float GetClampOfProgress(float InValue) const
	{
		return FMath::Clamp(InValue, 0.f, GetMaxProgressAsFloat());
	}

protected:
	//
	// TODO USE
	//

	// Achievement Title
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = ">Runtime")
	FText Title;

	// Locked description of the achievement
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = ">Runtime")
	FText LockedDesc;

	// Unlocked description of the achievement
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = ">Runtime")
	FText UnlockedDesc;

	// The Date/Time the achievement was unlocked
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = ">Runtime")
	FDateTime UnlockTime{0};

	// NOTE: This is set from Unreal Online Subsystem
	// Progress towards completing the achievement in range: [0, 100]
	// Steam:
	//		It calls GetUserAchievement which returns if it is unlocked or not
	//		0 - locked
	//		1 - unlocked
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = ">Runtime")
	float Progress = 0.f;

	// Achievement set by steam
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = ">Runtime")
	bool bSetBySteam = false;

	//
	// TODO USE
	//


	// Unique ID of the achievement
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">Data")
	FName Name = NAME_None;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">Data")
	bool bIsHidden = false;

	// Simple achievements, those who can be locked/unlocked have this at 1
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">Data", meta = (ClampMin = "1", UIMin = "1"))
	int32 MaxProgress = 1;

	//
	// Steam
	//

	// On steam if MaxProgres != 1 we need to set this StatName to progress the achievement
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">Data")
	FName SteamUserStatName = NAME_None;

	// Type of user stat
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">Data")
	ESoSteamStatType SteamUserStatType = ESoSteamStatType::Int;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">Data")
	bool bIsOnSteam = true;
};

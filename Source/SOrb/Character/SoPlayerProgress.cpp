// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoPlayerProgress.h"

#include "EngineUtils.h"
#include "Misc/FileHelper.h"

#include "SplineLogic/SoPlayerSpline.h"
#include "Character/SoPlayerCharacterSheet.h"
#include "SoCharacter.h"
#include "Basic/SoGameMode.h"
#include "Basic/Helpers/SoDateTimeHelper.h"
#include "SaveFiles/SoWorldState.h"
#include "Levels/SoLevelHelper.h"
#include "Basic/SoGameInstance.h"
#include "Items/ItemTemplates/SoItemTemplateRuneStone.h"
#include "Online/SoOnlineRichPresence.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Online/Analytics/SoAnalytics.h"


DECLARE_STATS_GROUP(TEXT("SoPlayerProgress"), STATGROUP_SoPlayerProgress, STATCAT_Advanced);

DEFINE_LOG_CATEGORY_STATIC(LogSoProgress, All, All);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sets default values for this component's properties
USoPlayerProgress::USoPlayerProgress()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	// bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = true;

	bWantsInitializeComponent = true;

	OnlineRichPresence = CreateDefaultSubobject<USoOnlineRichPresence>(TEXT("SoOnlineRichPresence"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerProgress::InitializeComponent()
{
	Super::InitializeComponent();

	// Progress data is independent of anything else
	if (ASoGameMode* GameMode = ASoGameMode::GetInstance(this))
	{
		GameMode->OnPreSave.AddDynamic(this, &ThisClass::OnSave);
		GameMode->OnPostLoad.AddDynamic(this, &ThisClass::OnLoad);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerProgress::UninitializeComponent()
{
	if(ASoGameMode* GameMode = ASoGameMode::GetInstance(this))
	{
		GameMode->OnPreSave.RemoveDynamic(this, &ThisClass::OnSave);
		GameMode->OnPostLoad.RemoveDynamic(this, &ThisClass::OnLoad);
	}
	Super::UninitializeComponent();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerProgress::BeginPlay()
{
	Super::BeginPlay();
	SoOwner = CastChecked<ASoCharacter>(GetOwner());
	OwnerSplineLocation.SetSpline(nullptr);
	TempAnalyticsStats = {};
	GameInstance = USoGameInstance::GetInstance(this);
	check(GameInstance);

	if (OnlineRichPresence)
		OnlineRichPresence->SetOwner(this);

	// Do not collect spline stats if we are not in the chapter
	bCollectSplineStats = true;
	if (OnlineRichPresence)
		OnlineRichPresence->OnChapterChanged();

	SoOwner->OnPlayerRematerialized.AddDynamic(this, &ThisClass::OnPlayerRematerialize);
	SoOwner->OnPlayerRespawn.AddDynamic(this, &ThisClass::OnPlayerRespawn);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerProgress::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SoOwner->OnPlayerRematerialized.RemoveDynamic(this, &ThisClass::OnPlayerRematerialize);
	SoOwner->OnPlayerRespawn.RemoveDynamic(this, &ThisClass::OnPlayerRespawn);
	Super::EndPlay(EndPlayReason);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerProgress::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("TickComponent"), STAT_TickComponent, STATGROUP_SoPlayerProgress);

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const float UndilatedDeltaSeconds = USoDateTimeHelper::GetDeltaSeconds();
	IncreaseTotalPlayTimeSeconds(UndilatedDeltaSeconds);

	// Update other info
	UpdateChapter(UndilatedDeltaSeconds);
	UpdateSpline(UndilatedDeltaSeconds);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerProgress::OnSave()
{
	PreSaveEvent.Broadcast();
	FSoWorldState::Get().SetProgressStats(ProgressStats);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerProgress::OnLoad()
{
	PreLoadEvent.Broadcast();

	// Session ended
	ProgressStats = {};
	TempAnalyticsStats = {};
	ProgressStats = FSoWorldState::Get().GetProgressStats();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerProgress::OnPlayerRematerialize()
{
	// TODO check episode stuff?
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerProgress::OnPlayerRespawn()
{
	// TODO check episode stuff?
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerProgress::UpdateSpline(float DeltaSeconds)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("TickComponent - UpdateSpline"), STAT_UpdateSpline, STATGROUP_SoPlayerProgress);

	// TODO fix changing spline collecting stats from old one
	if (GameInstance->IsSavingOrLoading()) // || USoLevelHelper::IsAnyLevelLoading(this))
		return;

	FSoSplinePoint NewSplineLocation = ISoSplineWalker::Execute_GetSplineLocationI(SoOwner);

	ASoPlayerSpline* CurrentOwnerSpline = GetOwnerSpline();
	ASoPlayerSpline* NewOwnerSpline = Cast<ASoPlayerSpline>(NewSplineLocation.GetSpline());

	// Different spline?
	if (CurrentOwnerSpline != NewOwnerSpline)
	{
		if (NewOwnerSpline != nullptr)
		{
			if (OnlineRichPresence)
				OnlineRichPresence->OnSplineChanged();

			// New current
			CurrentOwnerSpline = NewOwnerSpline;

			// enter num increased
			const FName SplineName = NewOwnerSpline->GetSplineFName();
			if (bCollectSplineStats)
			{
				IncrementSplineEnterNum(SplineName);
				SetSplineLength(SplineName, NewOwnerSpline->GetSplineLength());
			}

			const FString LogEntry = FString::SanitizeFloat(ProgressStats.TotalPlayTimeSeconds) + ": Spline Entered: " + SplineName.ToString();
			UE_LOG(LogSoProgress, Display, TEXT("%s"), *LogEntry);
		}
	}

	// Tick
	if (bCollectSplineStats && CurrentOwnerSpline != nullptr)
	{
		const FName SplineName = CurrentOwnerSpline->GetSplineFName();

		// Time
		UpdateSplineFirstEnterTime(SplineName);
		IncreaseSplineTimeSpent(SplineName, DeltaSeconds);

		// FPS
		CurrentOwnerSpline->TickPerformanceStats(DeltaSeconds);
		CurrentOwnerSpline->UpdateCriticalPerformanceStats(NewSplineLocation.GetDistance());
	}

	// Tick Location
	OwnerSplineLocation = NewSplineLocation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerProgress::UpdateChapter(float DeltaSeconds)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("TickComponent - UpdateChapter"), STAT_UpdateChapter, STATGROUP_SoPlayerProgress);

	if (GameInstance->IsSavingOrLoading())
		return;

	const FName NewChapterName = USoLevelHelper::GetChapterNameFromObject(this);
	if (OwnerChapterName != NewChapterName)
	{
		if (!NewChapterName.IsNone())
		{
			UE_LOG(LogSoProgress, Display, TEXT("Chapter Entered: %s"), *NewChapterName.ToString());

			if (OnlineRichPresence)
				OnlineRichPresence->OnChapterChanged();
		}
	}


	// Tick Chapter
	OwnerChapterName = NewChapterName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoPlayerSpline* USoPlayerProgress::GetOwnerSpline()
{
	return Cast<ASoPlayerSpline>(OwnerSplineLocation.GetSpline());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerProgress::OnDeath(bool bSoulKeeperActive)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("OnDeath"), STAT_OnDeath, STATGROUP_SoPlayerProgress);

	IncrementTotalDeathNum();

	if (bSoulKeeperActive)
		IncrementTotalDeathWithSoulKeeperNum();
	else
		IncrementTotalDeathWithCheckpointNum();

	// create log entry:
	const float CurrentPlayTimeSeconds = GetCurrentPlayTimeSeconds();
	FString LogEntry = FString::SanitizeFloat(CurrentPlayTimeSeconds) + ": Death ";
	ASoPlayerSpline* Spline = GetOwnerSpline();
	if (Spline != nullptr)
		LogEntry += " in " + Spline->GetSplineName() + " spline at " + FString::SanitizeFloat(OwnerSplineLocation.GetDistance());
	else
		LogEntry += " InvalidSplineLocation";
	LogEntry += bSoulKeeperActive ? ", soul keeper used" : ", checkpoint used";
	UE_LOG(LogSoProgress, Display, TEXT("%s"), *LogEntry);

	// store data in spline
	if (Spline != nullptr)
	{
		const auto DeathEntry = FSoSplineDeathContext(OwnerSplineLocation.GetDistance(), CurrentPlayTimeSeconds, bSoulKeeperActive);
		AddSplineDeathEntry(Spline->GetSplineFName(), DeathEntry);
	}
	else
	{
		UE_LOG(LogSoProgress, Error, TEXT("Death registering  failed... invalid spline location"));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerProgress::OnSpellUsed(const USoItemTemplateRuneStone* UsedSpell)
{
	if (UsedSpell == nullptr)
		return;

	// TODO localization maybe breaks this?
	const FText EnglishName = UsedSpell->GetItemName();
	SpellCasted(FName(*EnglishName.ToString()));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerProgress::OnEquippedItem(const USoItemTemplate* Item)
{
	if (Item)
		ProgressStats.EquipItem(Item->GetFName(), ProgressStats.TotalPlayTimeSeconds);
	// NOTE: Not interested in the TempAnalytics
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerProgress::OnUnEquippedItem(const USoItemTemplate* Item)
{
	if (Item)
		ProgressStats.UnEquipItem(Item->GetFName(), ProgressStats.TotalPlayTimeSeconds);
	// NOTE: Not interested in the TempAnalytics
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerProgress::OnHpLost(const FSoDmg& Damage)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("OnHpLost"), STAT_OnHpLost, STATGROUP_SoPlayerProgress);

	// input is reduced dmg already, not clamped to max hp tho
	ASoPlayerSpline* Spline = GetOwnerSpline();
	float PlayerPreHealth = SoOwner->GetPlayerCharacterSheet()->GetHealth();
	const FSoDmg BonusHealth = SoOwner->GetPlayerCharacterSheet()->GetBonushHealth();

	if (Damage.HasPhysical())
	{
		const float MaxPhysicalLoss = FMath::Min(Damage.Physical, PlayerPreHealth + BonusHealth.Physical);
		IncreaseTotalLostHP(ESoDmgType::Physical, MaxPhysicalLoss);
		if (bCollectSplineStats && Spline != nullptr)
			IncreaseSplineLostHP(Spline->GetSplineFName(), ESoDmgType::Physical, MaxPhysicalLoss);

		PlayerPreHealth = FMath::Min(PlayerPreHealth - Damage.Physical + BonusHealth.Physical, PlayerPreHealth);
	}

	if (Damage.HasMagical())
	{
		const float MaxMagicLoss = FMath::Min(Damage.Magical, PlayerPreHealth + BonusHealth.Magical);
		IncreaseTotalLostHP(ESoDmgType::Magic, MaxMagicLoss);
		if (bCollectSplineStats && Spline != nullptr)
			IncreaseSplineLostHP(Spline->GetSplineFName(), ESoDmgType::Magic, MaxMagicLoss);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerProgress::MarkCurrentEpisodeAsCompleted()
{
	StopGatheringPlayerData();
	// TODO
	// EpisodeStats.Progress = ESoEpisodeProgress::Completed;
	// FSoWorldState::Get().SetCurrentEpisode(EpisodeStats);
	// GameInstance->SaveGameForCurrentState();
	//
	// // Analytics
	// GameInstance->GetAnalytics()->RecordEpisodeCompleted(
	// 	FSoWorldState::Get().GetEpisodeName().ToString(),
	// 	EpisodeStats.TotalPlayTimeSeconds
	// );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoPlayerProgress::GetCurrentPlayTimeSeconds() const
{
	return ProgressStats.TotalPlayTimeSeconds;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerProgress::IncreaseTotalPlayTimeSeconds(float DeltaTime)
{
	ProgressStats.TotalPlayTimeSeconds += DeltaTime;
	TempAnalyticsStats.TotalPlayTimeSeconds += DeltaTime;

	// TODO special episodes
	// if (GameInstance->IsEpisode() && EpisodeStats.Progress == ESoEpisodeProgress::Started)
		// EpisodeStats.TotalPlayTimeSeconds += DeltaTime;
}

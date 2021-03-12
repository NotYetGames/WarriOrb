// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "SoActivity.h"
#include "SplineLogic/SoSplinePoint.h"
#include "SoATeleport.generated.h"

class ALevelSequenceActor;
class UAnimSequenceBase;
class ASoMarker;
class ASoPlayerSpline;
class ASoEnemy;

UENUM(BlueprintType)
enum class ESoATeleportState : uint8
{
	EATS_FadeOut,
	EATS_Faded,
	EATS_FadeIn,
	EATS_WaitForLevelToBeVisible,

	EATS_QuickTeleportPre,
	EATS_QuickTeleportPost,

	EATS_Done
};

UCLASS()
class SORB_API USoATeleport : public USoActivity
{
	GENERATED_BODY()

public:

	USoATeleport();

	virtual void Tick(float DeltaSeconds) override;

	// input
	virtual void JumpPressed() override {};
	virtual void Move(float Value) override {};
	virtual void ToggleWeapons() override {};

	/** true: already teleported, false: teleport state is used */
	UFUNCTION(BlueprintCallable, Category = SoCharacter)
	bool SetupTeleport(const FSoSplinePoint& SplineTarget,
					   float ZTarget,
					   bool bForceFade,
					   bool bLookBackwards,
					   bool bHideDuringTeleport = false,
					   bool bCamAlreadyOutFaded = false);

	/** tries to teleport the character without activity change, only works if there is no level change involved. Return true on success */
	UFUNCTION(BlueprintCallable, Category = SoCharacter)
	bool SetupQuickTeleport(const FSoSplinePoint& SplineTarget,
							float ZTarget,
							bool bLookBackwards);

	FORCEINLINE void SetSequence(ALevelSequenceActor* InLevelSequenceActor,
								 AActor* InCameraActor,
								 UAnimSequenceBase* InAnimation,
								 float CharHideTime = -1.0f,
								 ASoMarker* TelTargetAfterSequence = nullptr)
	{
		LevelSequenceActor = InLevelSequenceActor;
		CameraActor = InCameraActor;
		Animation = InAnimation;
		CharacterHideTimeDuringSequence = CharHideTime;
		TeleportTargetAfterSequence = TelTargetAfterSequence;
	}

	FORCEINLINE void ClearSequence()
	{
		LevelSequenceActor = nullptr;
		CameraActor = nullptr;
		Animation = nullptr;
	}

	UFUNCTION(BlueprintPure, Category = "SoCharacter")
	bool CanSwapPlayerWithSplineWalker(float MaxDistance);

	UFUNCTION(BlueprintCallable, Category = "SoCharacter")
	ASoEnemy* SwapPlayerWithSplineWalker(float MaxDistance, UParticleSystem* SwapVFX);

	UFUNCTION(BlueprintCallable, Category = "SoCharacter")
	ASoEnemy* TeleportBehindSplineWalker(float MaxDistance, UParticleSystem* VFX, float Distance = 100.0f);


	UFUNCTION(BlueprintPure, Category = "SoCharacter")
	bool CanTeleportBehindClosestEnemy(float MaxDistance);

	UFUNCTION(BlueprintCallable, Category = "SoCharacter")
	void TeleportBehindClosestEnemy(float MaxDistance);


	virtual bool CanInteract() const override { return false; }

	void SetResetCameraViewAfterPortal() { bResetCameraViewAfterPortal = true; }

	// switch only memory hack
	void ReduceTextureMemoryPool();
	void RestoreTextureMemoryPool();

protected:

	virtual void OnEnter(USoActivity* OldActivity) override;
	virtual void OnExit(USoActivity* NewActivity) override;

	virtual bool CanBeInterrupted(EActivity DesiredState = EActivity::EA_Max) const override { return false; }

	virtual bool DecreaseHealth(const FSoDmg& Damage) override { return true; }

	UFUNCTION(BlueprintPure, Category = UI)
	bool ShouldHideGameOverUI() const { return TeleportState != ESoATeleportState::EATS_FadeOut; }

	virtual void UpdateCamera(float DeltaSeconds) override;

	virtual bool CanBeArmedState() const override { return true; }
	virtual bool ShouldDecreaseCollision() const;

	FVector GetCharRelevantDirection() const;

	void StartWaitForLevel();
	void UpdateWaitForLevel();

protected:
	ESoATeleportState TeleportState;

	UPROPERTY(BlueprintReadOnly)
	FSoSplinePoint StoredSplineTarget;

	float StoredZTarget;
	bool bStoredLookBackwards;
	bool bMusicSwitchInProgress;

	float Counter;
	bool bRevive;

	int32 FrameCounter = -1;
	TSet<FName> LevelsToHide;

	bool bHide;

	bool bLevelSwitchHappened = false;

	UPROPERTY(BlueprintReadWrite)
	ALevelSequenceActor* LevelSequenceActor = nullptr;

	UPROPERTY(BlueprintReadWrite)
	TSoftObjectPtr<ALevelSequenceActor> LevelSequenceActorPtr;

	UPROPERTY(BlueprintReadWrite)
	ASoMarker* TeleportTargetAfterSequence = nullptr;

	UPROPERTY(BlueprintReadWrite)
	AActor* CameraActor = nullptr;

	/** if set it is triggered on the character location when the fade in starts */
	UPROPERTY(BlueprintReadWrite)
	UParticleSystem* ParticleAfterPortal = nullptr;

	UPROPERTY(BlueprintReadWrite)
	UAnimSequenceBase* Animation = nullptr;

	float CharacterHideTimeDuringSequence = -1.0f;

	UPROPERTY(BlueprintReadWrite)
	bool bResetCameraViewAfterPortal = false;

	UPROPERTY()
	const ASoPlayerSpline* StoredOldSpline;

	UPROPERTY()
	const ASoPlayerSpline* StoredNewSpline;

#if PLATFORM_SWITCH
	bool bTexturePoolReduced = false;
#endif
};

// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MeshMaterialShaderType.h"

#include "CharacterBase/SoIMortalTypes.h"
#include "SplineLogic/SoSplinePoint.h"

#include "SoStaticHelper.generated.h"

class UBoxComponent;
class UDlgContext;
class APlayerController;
class USkeletalMeshComponent;
class ASoGameMode;
class FSoTimerManager;
class ASoCharacter;
class ASoCharacterBase;
class UAnimSequenceBase;
class UWidgetAnimation;
class USceneComponent;
class UPrimitiveComponent;
class ASoEnemy;
class ALODActor;
class UStaticMesh;
struct FHitResult;

/**
 *
 */
UCLASS()
class SORB_API USoStaticHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	// warning: player 0 is returned always, as this code was written for a singleplayer game

	UFUNCTION(BlueprintPure, Category = "SoStaticHelper", meta = (WorldContext = "WorldContextObject"))
	static ASoGameMode* GetSoGameMode(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "SoStaticHelper")
	static ASoGameMode* GetSoGameModeFromContext(const UObject* Context);

	static FSoTimerManager& GetSoTimerManager(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "SoStaticHelper", meta = (WorldContext = "WorldContextObject"))
	static FName GetNameFromActor(const UObject* WorldContextObject, FString PreFix);

	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"))
	static bool IsActorInPlay(const UObject* WorldContextObject);


	// Only the first, this is a single player game after all
	UFUNCTION(BlueprintPure, Category = "SoStaticHelper", meta = (WorldContext = "WorldContextObject"))
	static APlayerController* GetPlayerController(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "SoStaticHelper", meta = (WorldContext = "WorldContextObject"))
	static AActor* GetPlayerCharacterAsActor(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "SoStaticHelper", meta = (WorldContext = "WorldContextObject"))
	static ASoCharacter* GetPlayerCharacterAsSoCharacter(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "SoStaticHelper", meta = (WorldContext = "WorldContextObject"))
	static ASoCharacterBase* GetPlayerCharacterAsSoCharacterBase(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "SoStaticHelper", meta = (WorldContext = "WorldContextObject"))
	static AActor* GetPlayerCharacterAsNullptr(const UObject* WorldContextObject);

	// meta = (WorldContext = "WorldContextObject")
	UFUNCTION(BlueprintPure, Category = "SoStaticHelper")
	static ASoEnemy* GetClosestEnemyToLocation(const UObject* WorldContextObject, const FVector& WorldLocation, float MaxDistance, bool bPreferFlying);

	UFUNCTION(BlueprintPure, Category = "SoStaticHelper")
	static ASoEnemy* GetClosestEnemyInDirection(const UObject* WorldContextObject,
												const FVector& WorldLocation,
												float MaxDistance,
												const FVector& Direction,
												bool bOnlyMovable);

	UFUNCTION(BlueprintPure, Category = "SoStaticHelper")
	static void GetEnemiesInRadius(const UObject* WorldContextObject, const FVector& WorldLocation, float Radius, TArray<ASoEnemy*>& Enemies);

	UFUNCTION(BlueprintPure, Category = "SoStaticHelper")
	static bool GetPointOnBoxTopClosestToBone(FVector& OutLocation, const TArray<FName>& BoneList, USkeletalMeshComponent* Mesh, UBoxComponent* Box, float MaxAcceptedOffset = 100.0f);


	UFUNCTION(BlueprintPure, Category = "SoStaticHelper", meta = (WorldContext = "WorldContextObject"))
	static FSoSplinePoint GetPlayerSplineLocation(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "SoStaticHelper")
	static FName GetEnemyOverlapDamageImmunityName();

	UFUNCTION(BlueprintPure, Category = "SoStaticHelper")
	static FName GetLillianFormName();

	UFUNCTION(BlueprintPure, Category = "SoStaticHelper")
	static FName GetForceNoEnvMusicName();


	UFUNCTION(BlueprintPure, Category = "Health")
	static ESoDmgType GetDmgTypeFromDmg(const FSoDmg& Dmg);


	UFUNCTION(BlueprintCallable, Category = "SplinePoint")
	static bool CheckAndClaimLevelsInSplineLocation(const FSoSplinePoint& SplinePoint);

	UFUNCTION(BlueprintPure, Category = "SplinePoint")
	static float GetScaledAnimLength(UAnimSequenceBase* AnimSequenceBase);

	static void BuildAnimationMap(UObject* Widget, TMap<FName, UWidgetAnimation*>& AnimationMap);

	UFUNCTION(BlueprintCallable, Category = "SplinePoint")
	static void SetSceneComponentMobility(USceneComponent* SceneComponent, EComponentMobility::Type Mobility);

	UFUNCTION(BlueprintCallable, Category = "SplinePoint")
	static float GetLastRenderTimeOnScreen(UPrimitiveComponent* PrimitiveComponent);

	UFUNCTION(BlueprintPure, Category = "SplinePoint")
	static bool WasPrimitiveRenderedOnScreenRecently(UPrimitiveComponent* PrimitiveComponent, float Recently = 0.1f);

	/** Temp solution for voice acting based dialogues */
	static float CalcDialogueDisplayTime(const UDlgContext* Context);

	// Get the Name of the asset from it's path
	static FName GenerateNameFromAssetPath(UObject* Object);

	UFUNCTION(BlueprintPure, Category = "SkeletalMesh")
	static FName GetBoneNameFromBodyIndex(USkeletalMeshComponent* SkeletalMeshComponent, int32 BodyIndex);

	UFUNCTION(BlueprintCallable, Category = "Damage")
	static void DamageOverlappedMortals(UPrimitiveComponent* PrimitiveComponent,
										const FSoDmg& Damage,
										const FSoHitReactDesc& HitReactDesc);

	UFUNCTION(BlueprintCallable, Category = MeshHelper)
	static FVector GetClosestPointOnMesh(USkeletalMeshComponent* Mesh,
										 const FVector& SourceLocation);

	UFUNCTION(BlueprintCallable, Category = ObjectHelper)
	static void ClearAndInvalidateTimer(AActor* Actor, UPARAM(ref)FTimerHandle& Handle);

	UFUNCTION(BlueprintCallable, Category = ObjectHelper)
	static void CallMarkPackageDirtyOnObject(const UObject* Object);

#if WITH_EDITOR
	UFUNCTION(BlueprintCallable, Category = EditorHelper)
	static void AddActorAndMeshToHLODProxy(UObject* Proxy, ALODActor* InLODActor, UStaticMesh* InStaticMesh);
#endif

public:
	/** Names: */
	static const FName NameGold;
};

// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoTutorialDecal.h"

#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SpotLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"

#include "Basic/SoGameSingleton.h"
#include "SaveFiles/SoWorldState.h"
#include "Character/SoPlayerController.h"
#include "Settings/SoGameSettings.h"
#include "Settings/Input/SoInputHelper.h"
#include "SaveFiles/SoWorldStateBlueprint.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoTutorialDecal::ASoTutorialDecal()
{
	PrimaryActorTick.bCanEverTick = true;

	Sprite = CreateDefaultSubobject<UBillboardComponent>(TEXT("SoSprite"));
	SetRootComponent(Sprite);

	Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("SoArrow"));
	Arrow->SetupAttachment(RootComponent);

	DecalCharacter = CreateDefaultSubobject<UDecalComponent>(TEXT("SoDecalCharacter"));
	DecalCharacter->SetupAttachment(RootComponent);

	DecalKeyAnim0 = CreateDefaultSubobject<UDecalComponent>(TEXT("SoDecalKeyAnim0"));
	DecalKeyAnim0->SetupAttachment(RootComponent);
	DecalKeyAnim1 = CreateDefaultSubobject<UDecalComponent>(TEXT("SoDecalKeyAnim1"));
	DecalKeyAnim1->SetupAttachment(RootComponent);
	DecalKeyAnim2 = CreateDefaultSubobject<UDecalComponent>(TEXT("SoDecalKeyAnim2"));
	DecalKeyAnim2->SetupAttachment(RootComponent);

	SpotLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("SoSpotLight"));
	SpotLight->SetupAttachment(RootComponent);
}


#if WITH_EDITOR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoTutorialDecal::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (Cast<ULevel>(GetOuter()) != nullptr)
		Reinitialize();
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoTutorialDecal::BeginPlay()
{
	Super::BeginPlay();

	TriggerCounter = 0;
	if (ASoPlayerController* PlayerController = ASoPlayerController::GetInstance(this))
	{
		PlayerController->OnDeviceTypeChanged().AddDynamic(this, &ASoTutorialDecal::OnDeviceChanged);
		CurrentDeviceType = PlayerController->GetCurrentDeviceType();
		Reinitialize();
	}
	else
		Reinitialize();

	// this will also call postload which resets the decal
	USoEventHandlerHelper::SubscribeToSoPostLoad(this);

	USoGameSettings::Get().OnInputSettingsApplied.AddDynamic(this, &ASoTutorialDecal::OnInputSettingsApplied);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoTutorialDecal::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	USoEventHandlerHelper::UnsubscribeFromSoPostLoad(this);

	if (ASoPlayerController* PlayerController = ASoPlayerController::GetInstance(this))
		PlayerController->OnDeviceTypeChanged().RemoveDynamic(this, &ASoTutorialDecal::OnDeviceChanged);

	if (USoGameSettings* GameSettings = USoGameSettings::GetInstance())
		GameSettings->OnInputSettingsApplied.RemoveDynamic(this, &ASoTutorialDecal::OnInputSettingsApplied);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoTutorialDecal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CurrentValue = FMath::FInterpTo(CurrentValue, FadeTarget, DeltaSeconds, 1.0f);
	Update();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoTutorialDecal::Trigger_Implementation(const FSoTriggerData& TriggerData)
{
	if (TriggerData.SourceIdentifier == 0)
	{
		FadeTarget = 0.0f;
		if (bEnabledByDefault)
			USoWorldState::AddMyNameToSet(this);
	}
	else
	{
		TriggerCounter += 1;
		if (TriggerCounter == TriggerBeforeFade)
		{
			FadeTarget = 1.0f;

			if (!bEnabledByDefault)
				USoWorldState::AddMyNameToSet(this);
		}
	}

	SetActorTickEnabled(true);
	DecalCharacter->SetVisibility(true);
	SpotLight->SetVisibility(true);

	const TArray<FSoTutorialKeyParam>& KeyParams = (CurrentDeviceType == ESoInputDeviceType::Keyboard) ? KeyParamsKeyboard : KeyParamsController;
	UDecalComponent* KeyDecals[] = { DecalKeyAnim0, DecalKeyAnim1, DecalKeyAnim2 };
	for (int32 i = 0; i < 3; ++i)
	{
		const bool bUsed = KeyParams.IsValidIndex(i);
		KeyDecals[i]->SetVisibility(bUsed);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoTutorialDecal::HandleSoPostLoad_Implementation()
{
	const bool bChanged = USoWorldState::IsMyNameInSet(this);

	TriggerCounter = 0;
	FadeTarget = (bEnabledByDefault == bChanged) ? 0.0f : 1.0f;
	CurrentValue = FadeTarget;
	Update();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoTutorialDecal::OnDeviceChanged(ESoInputDeviceType NewDeviceType)
{
	CurrentDeviceType = NewDeviceType;
	Reinitialize();
	Update();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoTutorialDecal::OnInputSettingsApplied()
{
	Reinitialize();
	Update();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoTutorialDecal::Reinitialize()
{
	// main decal
	{
		UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(DecalCharacter->GetDecalMaterial());
		if (DynamicMaterial == nullptr)
		{
			DecalCharacter->SetDecalMaterial(BaseMaterial);
			DynamicMaterial = DecalCharacter->CreateDynamicMaterialInstance();
		}
		if (DynamicMaterial != nullptr)
		{
			DynamicMaterial->SetScalarParameterValue(FName("PeriodTime"), PeriodTime);
			DynamicMaterial->SetScalarParameterValue(FName("TextureUsagePercent"), UsedAtlasSlotCount / static_cast<float>(RowNum * ColNum));
			DynamicMaterial->SetScalarParameterValue(FName("RowNum"), static_cast<float>(RowNum));
			DynamicMaterial->SetScalarParameterValue(FName("ColNum"), static_cast<float>(ColNum));
			DynamicMaterial->SetScalarParameterValue(FName("Exponent"), Exponent);
			DynamicMaterial->SetTextureParameterValue(FName("Texture"), DecalTexture);
		}
	}

	const TArray<FSoTutorialKeyParam>& KeyParams = (CurrentDeviceType == ESoInputDeviceType::Keyboard) ? KeyParamsKeyboard : KeyParamsController;
	UDecalComponent* KeyDecals[] = { DecalKeyAnim0, DecalKeyAnim1, DecalKeyAnim2 };
	for (int32 i = 0; i < 3; ++i)
	{
		const bool bUsed = KeyParams.IsValidIndex(i);
		KeyDecals[i]->SetVisibility(bUsed);
		if (bUsed)
		{
			bool bSquare = true;
			KeyDecals[i]->SetRelativeLocation(KeyParams[i].RelativeLocation);

			UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(KeyDecals[i]->GetDecalMaterial());
			if (DynamicMaterial == nullptr || DynamicMaterial->Parent != (KeyParams[i].bDoublePress ? KeyMaterialDoublePress : KeyMaterial))
			{
				KeyDecals[i]->SetDecalMaterial(KeyParams[i].bDoublePress ? KeyMaterialDoublePress : KeyMaterial);
				DynamicMaterial = KeyDecals[i]->CreateDynamicMaterialInstance();
			}
			if (DynamicMaterial != nullptr)
			{
				DynamicMaterial->SetScalarParameterValue(FName("PeriodTime"), PeriodTime * KeyPeriodMultiplier);

				DynamicMaterial->SetScalarParameterValue(FName("PressedStart"), KeyParams[i].PressIntervals.X);
				DynamicMaterial->SetScalarParameterValue(FName("PressedEnd"), KeyParams[i].PressIntervals.Y);

				UTexture2D* FirstTexture = USoGameSingleton::GetIconForInputActionNameType(KeyParams[i].InputAction, CurrentDeviceType, true);
				DynamicMaterial->SetTextureParameterValue(FName("PressedTexture"), FirstTexture);
				if (FirstTexture != nullptr)
					bSquare = FirstTexture->GetSizeX() == FirstTexture->GetSizeY();

				UTexture2D* SecondTexture = nullptr;
				if (KeyParams[i].bUseSecondaryInputActionInsteadOfUnpressed)
				{
					SecondTexture = USoGameSingleton::GetIconForInputActionNameType(KeyParams[i].SecondaryInputAction, CurrentDeviceType, true);
				}
				else
				{
					// check for special case: unpressed movement stick
					if (CurrentDeviceType != ESoInputDeviceType::Keyboard &&
						(KeyParams[i].InputAction == ESoInputActionNameType::IANT_MoveLeft || KeyParams[i].InputAction == ESoInputActionNameType::IANT_MoveRight))
					{
						FInputActionKeyMapping KeyMapping;
						const TArray<FInputActionKeyMapping> ActionMappings = USoGameSettings::Get().GetGamepadInputActionMappingsForActionName(
							FSoInputActionName::MoveLeft,
							false
						);
						if (USoInputHelper::GetFirstInputActionKeyMapping(ActionMappings, KeyMapping))
						{
							const FName ThumbStickDirection = KeyMapping.Key.GetFName();
							const FName ThumbStick = FSoInputKey::GetThumbStickForStickDirection(ThumbStickDirection);

							FSoInputGamepadKeyTextures Textures;
							if (USoGameSingleton::GetGamepadKeyTexturesFromKeyName(ThumbStick, false, Textures))
								SecondTexture = Textures.GetTextureForDeviceType(CurrentDeviceType);
						}

					}
					else
						SecondTexture = USoGameSingleton::GetIconForInputActionNameType(KeyParams[i].InputAction, CurrentDeviceType, false); // grab unpressed key
				}

				DynamicMaterial->SetTextureParameterValue(FName("UnpressedTexture"), SecondTexture);
			}
			KeyDecals[i]->DecalSize = GetDecalSize(bSquare);
			KeyDecals[i]->PushSelectionToProxy();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector ASoTutorialDecal::GetDecalSize(bool bSquared) const
{
	if (CurrentDeviceType == ESoInputDeviceType::Keyboard)
		return bSquared ? FVector(16.0f, 32.0f, 32.0f) : FVector(16.0f, 32.0f, 64.0f);

	return FVector(16.0f, 48.0f, 48.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoTutorialDecal::SetScalarOnDecalSafe(UDecalComponent* Decal, FName Name, float Value)
{
	if (Decal != nullptr)
	{
		if (UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(Decal->GetDecalMaterial()))
			DynamicMaterial->SetScalarParameterValue(Name, Value);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoTutorialDecal::Update()
{
	SpotLight->SetIntensity(CurrentValue);
	static const FName OpacityName = FName("Opacity");
	SetScalarOnDecalSafe(DecalCharacter, OpacityName, CurrentValue);
	SetScalarOnDecalSafe(DecalKeyAnim0, OpacityName, CurrentValue);
	SetScalarOnDecalSafe(DecalKeyAnim1, OpacityName, CurrentValue);
	SetScalarOnDecalSafe(DecalKeyAnim2, OpacityName, CurrentValue);

	if (fabs(CurrentValue - FadeTarget) < KINDA_SMALL_NUMBER)
	{
		SetActorTickEnabled(false);
		if (CurrentValue < 0.1f)
		{
			DecalCharacter->SetVisibility(false);
			DecalKeyAnim0->SetVisibility(false);
			DecalKeyAnim1->SetVisibility(false);
			DecalKeyAnim2->SetVisibility(false);
			SpotLight->SetVisibility(false);
		}
	}
}

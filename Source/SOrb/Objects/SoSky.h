// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Levels/SoEnvironmentPreset.h"

#include "SoSky.generated.h"

UCLASS()
class SORB_API ASoSky : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASoSky();

	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void Tick(float DeltaTime) override;

	// Setter for previewed values
	void SetPreviewedPreset0(FName InPresetName) { PreviewedPreset0 = InPresetName; }
	void SetPreviewedPreset1(FName InPresetName) { PreviewedPreset1 = InPresetName; }
	void SetBlendValue(float InBlendValue) { BlendValue = InBlendValue; }

	// Update the sky with the values for the previewed sky
	void UpdatePreviewedSky()
	{
		InitializeSoSky();
		UpdateSky(PreviewedPreset0, PreviewedPreset1, BlendValue);
	}

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void InitializeSoSky();

	void UpdateSky(FName FirstPresetName, FName SecondPresetName, float LerpValue);


	void UpdateSkyMesh(const FSoSkyPreset& SrcSky, const FSoSkyPreset& DstSky, float LerpValue);
	void UpdateSkyMesh(const FSoSkyPreset& Preset);

	void UpdateSunParams(const FSoSunPreset& SrcSun, const FSoSunPreset& DstSun, float LerpValue);
	void UpdateSunParams(const FSoSunPreset& Preset);

	void UpdateSkyLight(const FSoSkyLightPreset& SrcLight, const FSoSkyLightPreset& DstLight, float LerpValue);
	void UpdateSkyLight(const FSoSkyLightPreset& Preset);

	void UpdateFog(const FSoExponentialHeightFogPreset& SrcFog, const FSoExponentialHeightFogPreset& DstFog, float LerpValue);
	void UpdateFog(const FSoExponentialHeightFogPreset& Preset);

	UTextureCube* GetTextureCube(FSoftObjectPath Path);

public:
	static const FName DefaultPresetName;

protected:

	/** name of the preset previewed in editor if BlendValue = 0.0f */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Preview)
	FName PreviewedPreset0 = "Default";

	/** name of the preset previewed in editor if BlendValue = 1.0f */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Preview)
	FName PreviewedPreset1 = "Default";

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Preview)
	float BlendValue = 0.0f;

	/** on act3 it ruins some preset so it is disabled by default, use with caution! */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Preview)
	bool bEnableBlendedPreview = false;


	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<FName, FSoEnvironmentPreset> Presets;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<FName, FSoEnvironmentPreset> PresetsWithoutVolumetricFog;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	USkyLightComponent* SkyLight;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UStaticMeshComponent* SkySphereMesh;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UExponentialHeightFogComponent* FogComponent;

	UMaterialInstanceDynamic* DynamicMaterial = nullptr;

	bool bWasSkyCubemapBlended = false;

	// base color
	const FName BaseColorName = "BaseColor";
	const FName BrightnessName = "Brightness";
	const FName GradientAddName = "GradientAdd";
	const FName GradientMultiplyName = "GradientMultiply";
	const FName GradientPowerName = "GradientPower";
	// Cloud
	const FName CloudColorName = "CloudColor";
	const FName CloudLerpMultiplyName = "CloudLerpMultiply";
	const FName CloudLerpPowerName = "CloudLerpPower";
	const FName CloudLerpUVName = "CloudLerpUV";
	const FName CloudOpacityName = "CloudOpacity";
	const FName CloudSpeedName = "CloudSpeed";
	const FName CloudMoreCloudName = "MoreClouds";
	// Cubemap
	const FName CubemapStrengthName = "CubemapStrength";
	const FName SkyTexLerpName = "SkyTexLerp";
	const FName SourceTextureName = "SourceTexture";
	const FName DestTextureName = "DestTexture";
	// Stars
	const FName StarBrightnessName = "StarBrightness";
	const FName StarColorName = "StarColor";
	const FName StarShineMaskUVName = "StarShineMaskUV";
	const FName StarShineMinName = "StarShineMin";
	const FName StarShineMaxName = "StarShineMax";
	const FName StarUVName = "StarUV";

	const FName CubeMapLerpName = "CubemapLerp";

	const FName SunCloudColorName = "SunCloudColor";
	const FName SunColorName = "SunColor";
	const FName SunGradientMultiplyName = "SunGradientMultiply";
	const FName SunGradientPowerName = "SunGradientPower";
	const FName SunGradientRadiusName = "SunGradientRadius";
	const FName SunGradientXName = "SunGradientX";
	const FName SunGradientYName = "SunGradientY";
	const FName SunMultiplyName = "SunMultiply";
	const FName SunRadiusName = "SunRadius";
};

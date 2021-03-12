// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoSky.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/TextureCube.h"

#include "Character/SoCharacter.h"
#include "Character/SoCharStates/SoASkyControlEdit.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Settings/SoGameSettings.h"
#include "SplineLogic/SoPlayerSpline.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoSky, All, All);


const FName ASoSky::DefaultPresetName = FName("Default");

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoSky::ASoSky()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SoSkyLight"));
	SkySphereMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SoSkySphereMesh"));
	FogComponent = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("FogComponent"));

	RootComponent = SkySphereMesh;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoSky::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	DynamicMaterial = SkySphereMesh->CreateAndSetMaterialInstanceDynamic(0);

	if (!bEnableBlendedPreview)
	{
		BlendValue = FMath::Clamp(BlendValue, 0.0f, 1.0f);
		if (BlendValue < 0.5f)
			BlendValue = 0.0f;
		else
			BlendValue = 1.0f;
	}

	InitializeSoSky();
	UpdateSky(PreviewedPreset0, PreviewedPreset1, BlendValue);
}

#if WITH_EDITOR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoSky::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdatePreviewedSky();
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoSky::InitializeSoSky()
{
	if (!Presets.Contains(DefaultPresetName))
		Presets.Add(DefaultPresetName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoSky::BeginPlay()
{
	Super::BeginPlay();

	if (DynamicMaterial == nullptr)
		DynamicMaterial = SkySphereMesh->CreateAndSetMaterialInstanceDynamic(0);

	TArray<FName> PresetNames;
	for (const auto& Pair : Presets)
		PresetNames.Add(Pair.Key);

	if (ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
		Character->SoASkyControlEdit->InitializePresets(PresetNames);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoSky::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (const ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
	{
		const FSoSplinePoint SplineLocation = ISoSplineWalker::Execute_GetSplineLocationI(Character);
		const ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(SplineLocation.GetSpline());

		if (PlayerSpline != nullptr)
		{
			const FSoSkyControlValue Value = PlayerSpline->GetSkyControlPoints().GetInterpolated(SplineLocation);
			UpdateSky(Value.FirstPreset, Value.SecondPreset, Value.SecondWeight);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoSky::UpdateSky(FName FirstPresetName, FName SecondPresetName, float LerpValue)
{
	if (DynamicMaterial == nullptr)
		return;

	LerpValue = FMath::Clamp(LerpValue, 0.0f, 1.0f);

	// preset selection: look for volumetric free versions if it is turned off
	FSoEnvironmentPreset* FirstPreset = nullptr;
	FSoEnvironmentPreset* SecondPreset = nullptr;
	if (!USoGameSettings::Get().IsVolumetricFogEnabled())
	{
		FirstPreset = PresetsWithoutVolumetricFog.Find(FirstPresetName);
		SecondPreset = PresetsWithoutVolumetricFog.Find(SecondPresetName);
	}
	if (FirstPreset == nullptr)
		FirstPreset = Presets.Find(FirstPresetName);
	if (SecondPreset == nullptr)
		SecondPreset = Presets.Find(SecondPresetName);

	if (FirstPreset == nullptr || SecondPreset == nullptr)
	{
		UE_LOG(LogSoSky, Warning, TEXT("Failed to find sky preset %s or %s"), *FirstPresetName.ToString(), *SecondPresetName.ToString());
		return;
	}

	if (LerpValue < 0.0001f || FirstPresetName == SecondPresetName)
	{
		UpdateSkyMesh(FirstPreset->SkyParams);
		UpdateSunParams(FirstPreset->SunParams);
		UpdateSkyLight(FirstPreset->SkyLightParams);
		UpdateFog(FirstPreset->FogParams);
	}
	else if (LerpValue > 0.9999f)
	{
		UpdateSkyMesh(SecondPreset->SkyParams);
		UpdateSunParams(SecondPreset->SunParams);
		UpdateSkyLight(SecondPreset->SkyLightParams);
		UpdateFog(SecondPreset->FogParams);
	}
	else
	{
		UpdateSkyMesh(FirstPreset->SkyParams, SecondPreset->SkyParams, LerpValue);
		UpdateSunParams(FirstPreset->SunParams, SecondPreset->SunParams, LerpValue);
		UpdateSkyLight(FirstPreset->SkyLightParams, SecondPreset->SkyLightParams, LerpValue);
		UpdateFog(FirstPreset->FogParams, SecondPreset->FogParams, LerpValue);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoSky::UpdateSkyMesh(const FSoSkyPreset& SrcSky, const FSoSkyPreset& DstSky, float LerpValue)
{
	// base color
	DynamicMaterial->SetVectorParameterValue(BaseColorName, FMath::Lerp(SrcSky.BaseColor, DstSky.BaseColor, LerpValue));
	DynamicMaterial->SetScalarParameterValue(BrightnessName, FMath::Lerp(SrcSky.Brightness, DstSky.Brightness, LerpValue));
	DynamicMaterial->SetScalarParameterValue(GradientAddName, FMath::Lerp(SrcSky.GradientAdd, DstSky.GradientAdd, LerpValue));
	DynamicMaterial->SetScalarParameterValue(GradientMultiplyName, FMath::Lerp(SrcSky.GradientMultiply, DstSky.GradientMultiply, LerpValue));
	DynamicMaterial->SetScalarParameterValue(GradientPowerName, FMath::Lerp(SrcSky.GradientPower, DstSky.GradientPower, LerpValue));

	// cloud
	DynamicMaterial->SetVectorParameterValue(CloudColorName, FMath::Lerp(SrcSky.CloudColor, DstSky.CloudColor, LerpValue));
	DynamicMaterial->SetScalarParameterValue(CloudLerpMultiplyName, FMath::Lerp(SrcSky.CloudLerpMultiply, DstSky.CloudLerpMultiply, LerpValue));
	DynamicMaterial->SetScalarParameterValue(CloudLerpPowerName, FMath::Lerp(SrcSky.CloudLerpPower, DstSky.CloudLerpPower, LerpValue));
	DynamicMaterial->SetScalarParameterValue(CloudLerpUVName, FMath::Lerp(SrcSky.CloudLerpUV, DstSky.CloudLerpUV, LerpValue));
	DynamicMaterial->SetScalarParameterValue(CloudOpacityName, FMath::Lerp(SrcSky.CloudOpacity, DstSky.CloudOpacity, LerpValue));
	DynamicMaterial->SetScalarParameterValue(CloudSpeedName, FMath::Lerp(SrcSky.CloudSpeed, DstSky.CloudSpeed, LerpValue));
	DynamicMaterial->SetScalarParameterValue(CloudMoreCloudName, FMath::Lerp(SrcSky.CloudMoreClouds, DstSky.CloudMoreClouds, LerpValue));

	// Cubemap
	DynamicMaterial->SetScalarParameterValue(CubemapStrengthName, FMath::Lerp(SrcSky.CubemapStrength, DstSky.CubemapStrength, LerpValue));
	DynamicMaterial->SetScalarParameterValue(SkyTexLerpName, FMath::Lerp(SrcSky.SkyTexLerp, DstSky.SkyTexLerp, LerpValue));
	DynamicMaterial->SetTextureParameterValue(SourceTextureName, GetTextureCube(SrcSky.TexturePath));
	DynamicMaterial->SetTextureParameterValue(DestTextureName, GetTextureCube(DstSky.TexturePath));

	// Stars
	DynamicMaterial->SetVectorParameterValue(StarColorName, FMath::Lerp(SrcSky.StarColor, DstSky.StarColor, LerpValue));

	DynamicMaterial->SetScalarParameterValue(StarBrightnessName, FMath::Lerp(SrcSky.StarBrightness, DstSky.StarBrightness, LerpValue));
	DynamicMaterial->SetScalarParameterValue(StarShineMaskUVName, FMath::Lerp(SrcSky.StarShineMaskUV, DstSky.StarShineMaskUV, LerpValue));
	DynamicMaterial->SetScalarParameterValue(StarShineMinName, FMath::Lerp(SrcSky.StarShineMin, DstSky.StarShineMin, LerpValue));
	DynamicMaterial->SetScalarParameterValue(StarShineMaxName, FMath::Lerp(SrcSky.StarShineMax, DstSky.StarShineMax, LerpValue));
	DynamicMaterial->SetScalarParameterValue(StarUVName, FMath::Lerp(SrcSky.StarUV, DstSky.StarUV, LerpValue));

	DynamicMaterial->SetScalarParameterValue(CubeMapLerpName, LerpValue);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoSky::UpdateSkyMesh(const FSoSkyPreset& Preset)
{
	// base color
	DynamicMaterial->SetVectorParameterValue(BaseColorName, Preset.BaseColor);
	DynamicMaterial->SetScalarParameterValue(BrightnessName, Preset.Brightness);
	DynamicMaterial->SetScalarParameterValue(GradientAddName, Preset.GradientAdd);
	DynamicMaterial->SetScalarParameterValue(GradientMultiplyName, Preset.GradientMultiply);
	DynamicMaterial->SetScalarParameterValue(GradientPowerName, Preset.GradientPower);

	// cloud
	DynamicMaterial->SetVectorParameterValue(CloudColorName, Preset.CloudColor);
	DynamicMaterial->SetScalarParameterValue(CloudLerpMultiplyName, Preset.CloudLerpMultiply);
	DynamicMaterial->SetScalarParameterValue(CloudLerpPowerName, Preset.CloudLerpPower);
	DynamicMaterial->SetScalarParameterValue(CloudLerpUVName, Preset.CloudLerpUV);
	DynamicMaterial->SetScalarParameterValue(CloudOpacityName, Preset.CloudOpacity);
	DynamicMaterial->SetScalarParameterValue(CloudSpeedName, Preset.CloudSpeed);
	DynamicMaterial->SetScalarParameterValue(CloudMoreCloudName, Preset.CloudMoreClouds);

	// Cubemap
	DynamicMaterial->SetScalarParameterValue(CubemapStrengthName, Preset.CubemapStrength);
	DynamicMaterial->SetScalarParameterValue(SkyTexLerpName, Preset.SkyTexLerp);
	DynamicMaterial->SetTextureParameterValue(SourceTextureName, GetTextureCube(Preset.TexturePath));
	DynamicMaterial->SetTextureParameterValue(DestTextureName, GetTextureCube(Preset.TexturePath));

	// Stars
	DynamicMaterial->SetVectorParameterValue(StarColorName, Preset.StarColor);

	DynamicMaterial->SetScalarParameterValue(StarBrightnessName, Preset.StarBrightness);
	DynamicMaterial->SetScalarParameterValue(StarShineMaskUVName, Preset.StarShineMaskUV);
	DynamicMaterial->SetScalarParameterValue(StarShineMinName, Preset.StarShineMin);
	DynamicMaterial->SetScalarParameterValue(StarShineMaxName, Preset.StarShineMax);
	DynamicMaterial->SetScalarParameterValue(StarUVName, Preset.StarUV);

	DynamicMaterial->SetScalarParameterValue(CubeMapLerpName, 0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoSky::UpdateSunParams(const FSoSunPreset& SrcSun, const FSoSunPreset& DstSun, float LerpValue)
{
	DynamicMaterial->SetVectorParameterValue(SunCloudColorName, FMath::Lerp(SrcSun.CloudColor, DstSun.CloudColor, LerpValue));
	DynamicMaterial->SetVectorParameterValue(SunColorName, FMath::Lerp(SrcSun.Color, DstSun.Color, LerpValue));

	DynamicMaterial->SetScalarParameterValue(SunGradientMultiplyName, FMath::Lerp(SrcSun.GradientMultiply, DstSun.GradientMultiply, LerpValue));
	DynamicMaterial->SetScalarParameterValue(SunGradientPowerName, FMath::Lerp(SrcSun.GradientPower, DstSun.GradientPower, LerpValue));
	DynamicMaterial->SetScalarParameterValue(SunGradientRadiusName, FMath::Lerp(SrcSun.GradientRadius, DstSun.GradientRadius, LerpValue));
	DynamicMaterial->SetScalarParameterValue(SunGradientXName, FMath::Lerp(SrcSun.GradientX, DstSun.GradientX, LerpValue));
	DynamicMaterial->SetScalarParameterValue(SunGradientYName, FMath::Lerp(SrcSun.GradientY, DstSun.GradientY, LerpValue));
	DynamicMaterial->SetScalarParameterValue(SunMultiplyName, FMath::Lerp(SrcSun.SunMultiply, DstSun.SunMultiply, LerpValue));
	DynamicMaterial->SetScalarParameterValue(SunRadiusName, FMath::Lerp(SrcSun.SunRadius, DstSun.SunRadius, LerpValue));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoSky::UpdateSunParams(const FSoSunPreset& Preset)
{
	DynamicMaterial->SetVectorParameterValue(SunCloudColorName, Preset.CloudColor);
	DynamicMaterial->SetVectorParameterValue(SunColorName, Preset.Color);

	DynamicMaterial->SetScalarParameterValue(SunGradientMultiplyName, Preset.GradientMultiply);
	DynamicMaterial->SetScalarParameterValue(SunGradientPowerName, Preset.GradientPower);
	DynamicMaterial->SetScalarParameterValue(SunGradientRadiusName, Preset.GradientRadius);
	DynamicMaterial->SetScalarParameterValue(SunGradientXName, Preset.GradientX);
	DynamicMaterial->SetScalarParameterValue(SunGradientYName, Preset.GradientY);
	DynamicMaterial->SetScalarParameterValue(SunMultiplyName, Preset.SunMultiply);
	DynamicMaterial->SetScalarParameterValue(SunRadiusName, Preset.SunRadius);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoSky::UpdateSkyLight(const FSoSkyLightPreset& SrcLight, const FSoSkyLightPreset& DstLight, float LerpValue)
{
	SkyLight->SetCubemapBlend(GetTextureCube(SrcLight.CubeMapPath), GetTextureCube(DstLight.CubeMapPath), LerpValue);
	bWasSkyCubemapBlended = true;

	SkyLight->SetIntensity(FMath::Lerp(SrcLight.Intensity, DstLight.Intensity, LerpValue));

	const FLinearColor SrcColor = FLinearColor(SrcLight.LightColor);
	const FLinearColor DstColor = FLinearColor(DstLight.LightColor);
	SkyLight->SetLightColor(FMath::Lerp(SrcColor, DstColor, LerpValue));

	SkyLight->SetVolumetricScatteringIntensity(FMath::Lerp(SrcLight.VolumetricScatteringIntensity, DstLight.VolumetricScatteringIntensity, LerpValue));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoSky::UpdateSkyLight(const FSoSkyLightPreset& Preset)
{
	SkyLight->SetCubemap(GetTextureCube(Preset.CubeMapPath));
	if (bWasSkyCubemapBlended)
	{
		SkyLight->SetCubemapBlend(GetTextureCube(Preset.CubeMapPath), GetTextureCube(Preset.CubeMapPath), 0.0f);
		bWasSkyCubemapBlended = false;
	}

	SkyLight->SetIntensity(Preset.Intensity);
	SkyLight->SetLightColor(FLinearColor(Preset.LightColor));
	SkyLight->SetVolumetricScatteringIntensity(Preset.VolumetricScatteringIntensity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoSky::UpdateFog(const FSoExponentialHeightFogPreset& SrcFog, const FSoExponentialHeightFogPreset& DstFog, float LerpValue)
{
	FogComponent->SetFogDensity(FMath::Lerp(SrcFog.FogDensity, DstFog.FogDensity, LerpValue));
	FogComponent->SetFogMaxOpacity(FMath::Lerp(SrcFog.FogMaskOpacity, DstFog.FogMaskOpacity, LerpValue));
	FogComponent->SetFogInscatteringColor(FMath::Lerp(SrcFog.FogInscatteringColor, DstFog.FogInscatteringColor, LerpValue));
	FogComponent->SetFogHeightFalloff(FMath::Lerp(SrcFog.FogHeightFallof, DstFog.FogHeightFallof, LerpValue));
	FogComponent->SetFogCutoffDistance(FMath::Lerp(SrcFog.FogCutoffDistance, DstFog.FogCutoffDistance, LerpValue));
	FogComponent->SetVolumetricFog(SrcFog.bVolumetricFog || DstFog.bVolumetricFog);
	FogComponent->SetVolumetricFogExtinctionScale(FMath::Lerp(SrcFog.VolumetricFogExtinctionScale, DstFog.VolumetricFogExtinctionScale, LerpValue));
	FogComponent->SetVolumetricFogDistance(FMath::Lerp(SrcFog.VolumetricFogViewDistance, DstFog.VolumetricFogViewDistance, LerpValue));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoSky::UpdateFog(const FSoExponentialHeightFogPreset& Preset)
{
	FogComponent->SetFogDensity(Preset.FogDensity);
	FogComponent->SetFogMaxOpacity(Preset.FogMaskOpacity);
	FogComponent->SetFogInscatteringColor(Preset.FogInscatteringColor);
	FogComponent->SetFogHeightFalloff(Preset.FogHeightFallof);
	FogComponent->SetFogCutoffDistance(Preset.FogCutoffDistance);
	FogComponent->SetVolumetricFog(Preset.bVolumetricFog);
	FogComponent->SetVolumetricFogExtinctionScale(Preset.VolumetricFogExtinctionScale);
	FogComponent->SetVolumetricFogDistance(Preset.VolumetricFogViewDistance);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UTextureCube* ASoSky::GetTextureCube(FSoftObjectPath Path)
{
	UTextureCube* TextureCube = Cast<UTextureCube>(Path.ResolveObject());
	if (TextureCube != nullptr)
		return TextureCube;

	return Cast<UTextureCube>(Path.TryLoad());
}

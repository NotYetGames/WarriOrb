// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "SoEnvironmentPreset.generated.h"

class UTextureCube;

USTRUCT(BlueprintType, Blueprintable)
struct FSoSkyPreset
{
	GENERATED_USTRUCT_BODY()

public:

	// base color
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BaseColor)
	FLinearColor BaseColor = { 0.034f, 0.028f, 0.085f, 1.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BaseColor)
	float Brightness = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BaseColor)
	float GradientAdd = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BaseColor)
	float GradientMultiply = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BaseColor)
	float GradientPower = 3.0f;
	

	// Cloud
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Cloud)
	FLinearColor CloudColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Cloud)
	float CloudLerpMultiply = 1.08233f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Cloud)
	float CloudLerpPower = 1.276203f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Cloud)
	float CloudLerpUV = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Cloud)
	float CloudOpacity = 0.722936;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Cloud)
	float CloudSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Cloud)
	float CloudMoreClouds = 0.743801f;


	// Cubemap
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Cubemap)
	float CubemapStrength = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Cubemap)
	float SkyTexLerp = 1.0f;

	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Cubemap)
	// UTextureCube* Texture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Cubemap)
	FSoftObjectPath TexturePath;


	// Stars
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stars)
	float StarBrightness = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stars)
	FLinearColor StarColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stars)
	float StarShineMaskUV = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stars)
	float StarShineMin = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stars)
	float StarShineMax = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stars)
	float StarUV = 3.0f;
};

USTRUCT(BlueprintType, Blueprintable)
struct FSoSunPreset
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sun)
	FLinearColor CloudColor = { 0.0f, 0.0f, 0.0f, 1.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sun)
	FLinearColor Color = { 0.0f, 0.0f, 0.0f, 1.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sun)
	float GradientMultiply = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sun)
	float GradientPower = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sun)
	float GradientRadius = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sun)
	float GradientX = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sun)
	float GradientY = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sun)
	float SunMultiply = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sun)
	float SunRadius = 0.0f;
};


USTRUCT(BlueprintType, Blueprintable)
struct FSoSkyLightPreset
{
	GENERATED_USTRUCT_BODY()

public:

	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LightParam)
	// UTextureCube* CubeMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LightParam)
	FSoftObjectPath CubeMapPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LightParam)
	float Intensity = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LightParam)
	FColor LightColor = { 102, 153, 255, 255 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LightParam)
	float VolumetricScatteringIntensity = 1.0f;
};


USTRUCT(BlueprintType, Blueprintable)
struct FSoExponentialHeightFogPreset
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogParam)
	float FogDensity = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogParam)
	float FogMaskOpacity = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogParam)
	FLinearColor FogInscatteringColor = { 0.085f, 0.17f, 0.2, 1.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogParam)
	float FogHeightFallof = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogParam)
	float FogCutoffDistance = 20000000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogParam)
	bool bVolumetricFog = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogParam)
	float VolumetricFogExtinctionScale = 6.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogParam)
	float VolumetricFogViewDistance = 6000.0f;
};


USTRUCT(BlueprintType, Blueprintable)
struct FSoEnvironmentPreset
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSoSkyPreset SkyParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSoSunPreset SunParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSoSkyLightPreset SkyLightParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSoExponentialHeightFogPreset FogParams;
};

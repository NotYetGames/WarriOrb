// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "SoLocalizationHelper.generated.h"

// Enum that lists all the cultures of our game
UENUM(BlueprintType)
enum class ESoSupportedCulture : uint8
{
	// Unsupported basically
	Invalid = 0,

	English, // en also native culture

	Hungarian, // ro
	Romanian, // hu

	ChineseSimplified, // zh-Hans
	ChineseTraditional, // zh-Hant

	Russian, // ru
	French, // fr
	Spanish, // es
};

// Helper class for localization/internationalization stuff
// https://docs.unrealengine.com/en-US/Gameplay/Localization/ManageActiveCultureRuntime/index.html
UCLASS()
class SORB_API USoLocalizationHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Statically initialize to set some values
	static void Init();

	UFUNCTION(BlueprintCallable, Category=">Localization")
	static const TArray<ESoSupportedCulture>& GetAllConfigurableCultures() { return ConfigurableCultures; }

	// Converts Enum to IETF language tag
	UFUNCTION(BlueprintPure, Category=">Localization")
	static FString GetLanguageTagFromSupporterCulture(ESoSupportedCulture Culture)
	{
		switch (Culture)
		{
		case ESoSupportedCulture::Romanian:
			return TEXT("ro");
		case ESoSupportedCulture::Hungarian:
			return TEXT("hu");
		case ESoSupportedCulture::ChineseSimplified:
			return TEXT("zh-Hans");
		case ESoSupportedCulture::ChineseTraditional:
			return TEXT("zh-Hant");
		case ESoSupportedCulture::Russian:
			return TEXT("ru");
		case ESoSupportedCulture::French:
			return TEXT("fr");
		case ESoSupportedCulture::Spanish:
			return TEXT("es");

		// Sane default
		case ESoSupportedCulture::English:
		default:
			return TEXT("en");
		}
	}
	static void TestAllCultures();

	UFUNCTION(BlueprintPure, Category=">Localization|Steam")
	static ESoSupportedCulture GetSupportedCultureFromSteamAPILanguageCode(const FString& APILanguageCode)
	{
		// https://partner.steamgames.com/doc/store/localization#8
		if (APILanguageCode.Equals(TEXT("english"), ESearchCase::IgnoreCase))
		{
			return ESoSupportedCulture::English;
		}
		if (APILanguageCode.Equals(TEXT("romanian"), ESearchCase::IgnoreCase))
		{
			return ESoSupportedCulture::Romanian;
		}
		if (APILanguageCode.Equals(TEXT("hungarian"), ESearchCase::IgnoreCase))
		{
			return ESoSupportedCulture::Hungarian;
		}
		if (APILanguageCode.Equals(TEXT("schinese"), ESearchCase::IgnoreCase))
		{
			return ESoSupportedCulture::ChineseSimplified;
		}
		if (APILanguageCode.Equals(TEXT("tchinese"), ESearchCase::IgnoreCase))
		{
			return ESoSupportedCulture::ChineseTraditional;
		}
		if (APILanguageCode.Equals(TEXT("russian"), ESearchCase::IgnoreCase))
		{
			return ESoSupportedCulture::Russian;
		}
		if (APILanguageCode.Equals(TEXT("french"), ESearchCase::IgnoreCase))
		{
			return ESoSupportedCulture::French;
		}
		if (APILanguageCode.Equals(TEXT("spanish"), ESearchCase::IgnoreCase))
		{
			return ESoSupportedCulture::Spanish;
		}
		// Latin American spanish
		if (APILanguageCode.Equals(TEXT("latam"), ESearchCase::IgnoreCase))
		{
			return ESoSupportedCulture::Spanish;
		}

		return ESoSupportedCulture::Invalid;
	}

	UFUNCTION(BlueprintPure, Category=">Localization")
	static ESoSupportedCulture GetSupportedCultureFromLanguageTag(const FString& LanguageTag)
	{
		if (LanguageTag.StartsWith(TEXT("en")))
		{
			return ESoSupportedCulture::English;
		}
		if (LanguageTag.StartsWith(TEXT("ro")))
		{
			return ESoSupportedCulture::Romanian;
		}
		if (LanguageTag.StartsWith(TEXT("hu")))
		{
			return ESoSupportedCulture::Hungarian;
		}
		if (LanguageTag.StartsWith(TEXT("zh-Hans")))
		{
			return ESoSupportedCulture::ChineseSimplified;
		}
		if (LanguageTag.StartsWith(TEXT("zh-Hant")))
		{
			return ESoSupportedCulture::ChineseTraditional;
		}
		if (LanguageTag.StartsWith(TEXT("ru")))
		{
			return ESoSupportedCulture::Russian;
		}
		if (LanguageTag.StartsWith(TEXT("fr")))
		{
			return ESoSupportedCulture::French;
		}
		if (LanguageTag.StartsWith(TEXT("es")))
		{
			return ESoSupportedCulture::Spanish;
		}

		return ESoSupportedCulture::Invalid;
	}

	UFUNCTION(BlueprintPure, Category=">Localization")
	static FText GetDisplayTextFromSupportedCulture(ESoSupportedCulture CultureType, bool bInNativeCulture);

	// This basically sets the current culture everywhere
	// NOTE: This function is a sledgehammer, and will set both the language and locale, as well as clear out any asset group cultures that may be set.
	// NOTE: This clears Internationalization.AssetGroupCulture
	// Locale is what we represent time and numbers in
	// Language is what we see
	// NOTE: Does not work in editor after restart, set WARRIORB_NON_EDITOR_TEST = true
	UFUNCTION(BlueprintCallable, Category=">Localization")
	static bool SetCurrentCultureEverywhere(ESoSupportedCulture Culture, bool bSaveToConfig);

	// Did we save anything?
	static bool HasSavedCultureConfig();

	static FCulturePtr GetCultureFromLanguageTag(const FString& LanguageTag);
	static FCulturePtr GetCultureFromSupportedCulture(ESoSupportedCulture Culture)
	{
		return GetCultureFromLanguageTag(GetLanguageTagFromSupporterCulture(Culture));
	}

	UFUNCTION(BlueprintPure, Category=">Localization")
	static FString GetCurrentLanguageName();

	UFUNCTION(BlueprintPure, Category=">Localization")
	static ESoSupportedCulture GetCurrentLanguageNameType();

	// Set by steam
	UFUNCTION(BlueprintPure, Category=">Localization|Steam")
	static ESoSupportedCulture GetSteamCurrentLanguageNameType();

	// Helper function:
	// If val is
	// UFUNCTION(BlueprintPure)
	// static FText FloatAsTextPercentInteger(float Val);
protected:
	// By the users
	static TArray<ESoSupportedCulture> ConfigurableCultures;

	// Set in Initialize
	static bool bIsInitialized;
};

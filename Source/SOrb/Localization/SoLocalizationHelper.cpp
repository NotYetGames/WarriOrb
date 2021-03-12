// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoLocalizationHelper.h"

#include "Kismet/KismetInternationalizationLibrary.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"

#include "Misc/ConfigCacheIni.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/Helpers/SoStringHelper.h"
#include "Basic/Helpers/SoPlatformHelper.h"

bool USoLocalizationHelper::bIsInitialized = false;
TArray<ESoSupportedCulture> USoLocalizationHelper::ConfigurableCultures;

DEFINE_LOG_CATEGORY_STATIC(LogSoLocalization, All, All);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoLocalizationHelper::Init()
{
	if (bIsInitialized)
		return;
	bIsInitialized = true;

	ConfigurableCultures = {
		ESoSupportedCulture::English,
		ESoSupportedCulture::ChineseSimplified,
		ESoSupportedCulture::ChineseTraditional,
		ESoSupportedCulture::Russian,
		ESoSupportedCulture::French,
		ESoSupportedCulture::Spanish,
		ESoSupportedCulture::Hungarian,
		ESoSupportedCulture::Romanian
	};

#if WARRIORB_WITH_STEAM
	// We only use the steam language if the user did not already set one in the file
	if (USoPlatformHelper::IsSteamInitialized() && !HasSavedCultureConfig())
	{
		const ESoSupportedCulture SteamCulture = GetSteamCurrentLanguageNameType();

		// NOTE: we don't save it to the config, because the config is only for user set ingame language values
		SetCurrentCultureEverywhere(SteamCulture, false);

		UE_LOG(LogSoLocalization, Log, TEXT("Using Steam Language = %s"), *GetLanguageTagFromSupporterCulture(SteamCulture));
	}
#endif // WARRIORB_WITH_STEAM
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoLocalizationHelper::TestAllCultures()
{
	TArray<FString> StockCultureNames;
	FInternationalization::Get().GetCultureNames(StockCultureNames);

	for (ESoSupportedCulture SupportedCulture : ConfigurableCultures)
	{
		const FString LanguageTag = GetLanguageTagFromSupporterCulture(SupportedCulture);
		const FCulturePtr Culture = FInternationalization::Get().GetCulture(LanguageTag);
		check(Culture.IsValid());
		const FString DisplayName = Culture->GetDisplayName();
		const FString EnglishName = Culture->GetEnglishName();
		const FString NativeName = USoStringHelper::CapitalizeEachWordFromString(Culture->GetNativeName());

		UE_LOG(LogSoLocalization, Warning, TEXT("Tag = %s, DisplayName = %s, EnglishName = %s, Native = %s"), *LanguageTag, *DisplayName, *EnglishName, *NativeName)
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FText USoLocalizationHelper::GetDisplayTextFromSupportedCulture(ESoSupportedCulture CultureType, bool bInNativeCulture)
{
	const FCulturePtr Culture = GetCultureFromSupportedCulture(CultureType);
	check(Culture.IsValid());

	// Get the string as displayed in that culture
	if (bInNativeCulture)
	{
		const FString NativeName = USoStringHelper::CapitalizeEachWordFromString(Culture->GetNativeName());
		return FText::FromString(NativeName);
	}

	const FString DisplayName = Culture->GetDisplayName();
	return FText::FromString(DisplayName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoLocalizationHelper::SetCurrentCultureEverywhere(ESoSupportedCulture Culture, bool bSaveToConfig)
{
	const FString LanguageTag = GetLanguageTagFromSupporterCulture(Culture);

	// Copied from UKismetInternationalizationLibrary::SetCurrentCulture
	if (FInternationalization::Get().SetCurrentCulture(LanguageTag))
	{
		if (!WARRIORB_WITH_EDITOR && bSaveToConfig)
		{
			GConfig->SetString(TEXT("Internationalization"), TEXT("Culture"), *LanguageTag, GGameUserSettingsIni);
			GConfig->EmptySection(TEXT("Internationalization.AssetGroupCultures"), GGameUserSettingsIni);
			GConfig->Flush(false, GGameUserSettingsIni);
		}
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoLocalizationHelper::HasSavedCultureConfig()
{
	FString Culture;
	const bool bOk = GConfig->GetString(TEXT("Internationalization"), TEXT("Culture"), Culture, GGameUserSettingsIni);
	return bOk && Culture.Len() > 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FCulturePtr USoLocalizationHelper::GetCultureFromLanguageTag(const FString& LanguageTag)
{
	return FInternationalization::Get().GetCulture(LanguageTag);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString USoLocalizationHelper::GetCurrentLanguageName()
{
	return FInternationalization::Get().GetCurrentLanguage()->GetName();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoSupportedCulture USoLocalizationHelper::GetCurrentLanguageNameType()
{
	const FString LanguageTag = GetCurrentLanguageName();
	const ESoSupportedCulture Culture = GetSupportedCultureFromLanguageTag(LanguageTag);
	if (Culture == ESoSupportedCulture::Invalid)
	{
		UE_LOG(LogSoLocalization, Error, TEXT("GetCurrentLanguageNameType(): Invalid LanguageTag = %s, it is not in supported cultures. Defaulting to English"), *LanguageTag);
		return ESoSupportedCulture::English;
	}

	return Culture;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoSupportedCulture USoLocalizationHelper::GetSteamCurrentLanguageNameType()
{
	const FString APILanguageCode = USoPlatformHelper::GetSteamCurrentGameLanguage();
	const ESoSupportedCulture CultureType = GetSupportedCultureFromSteamAPILanguageCode(APILanguageCode);
	if (CultureType == ESoSupportedCulture::Invalid)
	{
		UE_LOG(LogSoLocalization, Error, TEXT("GetSteamCurrentLanguageNameType(): Invalid APILanguageCode = %s, it is not in supported cultures. Defaulting to English"), *APILanguageCode);
		return ESoSupportedCulture::English;
	}

	return CultureType;
}

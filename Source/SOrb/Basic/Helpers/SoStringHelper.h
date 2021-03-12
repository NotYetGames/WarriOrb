// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MeshMaterialShaderType.h"
#include "Misc/SecureHash.h"

#include "Items/SoItemTypes.h"
#include "CharacterBase/SoIMortalTypes.h"
#include "Settings/SoAudioSettingsTypes.h"

#include "SoStringHelper.generated.h"



/**
 *
 */
UCLASS()
class SORB_API USoStringHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category = StringHelper)
	static FString RepeatString(const FString& StringToRepeat, int32 RepeatCount, const FString& StringBetween);

	UFUNCTION(BlueprintPure, Category = StringHelper)
	static FString CapitalizeEachWordFromString(const FString& OriginalString);

	// From: https://kelheor.space/2018/11/12/how-to-encrypt-data-with-aes-256-in-ue4/
	UFUNCTION(BlueprintCallable, Category = "AES256")
	static FString AES256Encrypt(FString InputString, FString Key);
	UFUNCTION(BlueprintCallable, Category = "AES256")
	static FString AES256Decrypt(FString InputString, FString Key);

	// Gets the user ObjectName as it appears in the content browser
	// Works for blueprints and UMG widgets
	// And NOT Default_WidgetName_C_0
	// And NOT Default_WidgetName_BP_C_0
	static FString GetObjectBaseName(const UObject* Object);

	template<typename TEnum>
	static bool ConvertEnumToString(const FString& EnumName, TEnum EnumValue, bool bWithNameSpace, FString& OutEnumValue)
	{
		const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, *EnumName, true);
		if (!EnumPtr)
		{
			OutEnumValue = FString::Printf(TEXT("INVALID EnumName = `%s`"), *EnumName);
			return false;
		}

		OutEnumValue = bWithNameSpace ? EnumPtr->GetNameByIndex(static_cast<int32>(EnumValue)).ToString()
									  : EnumPtr->GetNameStringByIndex(static_cast<int32>(EnumValue));
		return true;
	}

	template<typename TEnum>
	static bool ConvertStringToEnum(const FString& String, const FString& EnumName, TEnum& OutEnumValue)
	{
		const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, *EnumName, true);
		if (!EnumPtr)
		{
			return false;
		}

		const int32 Index = EnumPtr->GetIndexByNameString(String);
		OutEnumValue = static_cast<TEnum>(Index);
		return true;
	}

	template<typename TEnum>
	static bool ConvertFNameToEnum(FName Name, const FString& EnumName, TEnum& OutEnumValue)
	{
		const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, *EnumName, true);
		if (!EnumPtr)
		{
			return false;
		}

		const int32 Index = EnumPtr->GetIndexByName(Name);
		OutEnumValue = static_cast<TEnum>(Index);
		return true;
	}

	// GetEnumValueAsString for ESoItemType (NOTE: this removes the namespace and prefix)
	static FString ItemTypeToFriendlyString(ESoItemType ItemType)
	{
		FString EnumValue;
		if (ConvertEnumToString<ESoItemType>(TEXT("ESoItemType"), ItemType, false, EnumValue))
			return EnumValue.Mid(4);

		return EnumValue;
	}

	// GetEnumValueAsString for ESoDamageType (NOTE: this removes the namespace and prefix)
	static FString DamageTypeToFriendlyString(ESoDmgType DamageType)
	{
		FString EnumValue;
		if (ConvertEnumToString<ESoDmgType>(TEXT("ESoDmgType"), DamageType, false, EnumValue))
			return EnumValue.Mid(4);

		return EnumValue;
	}

	static FString SFXToString(const ESoSFX Type)
	{
		FString EnumValue;
		if (ConvertEnumToString<ESoSFX>(TEXT("ESoSFX"), Type, false, EnumValue))
			return EnumValue;

		return EnumValue;
	}

	// NOTE: potential dangerous
	static FName FTextToFName(const FText& Text)
	{
		return FName(*Text.ToString());
	}

	//
	// SHA1
	//

	template<class DataType>
	static void UpdateSHA1_Data(FSHA1& HashState, const DataType& Data)
	{
		HashState.Update(reinterpret_cast<const uint8*>(&Data), sizeof(DataType));
	}

	template<class ElementType>
	static void UpdateSHA1_ArrayOfUObjects(FSHA1& HashState, const TArray<ElementType*>& Array)
	{
		// Use the pathname for the hash
		for (const auto* Elem : Array)
		{
			UpdateSHA1_UObject(HashState, Elem);
		}
	}

	template<class ElementType>
	static void UpdateSHA1_Array(FSHA1& HashState, const TArray<ElementType>& Array)
	{
		// Use the pathname for the hash
		HashState.Update(reinterpret_cast<const uint8*>(Array.GetData()), Array.Num() * Array.GetTypeSize());
	}

	FORCEINLINE static void UpdateSHA1_UObject(FSHA1& HashState, const UObject* Object)
	{
		if (IsValid(Object))
		{
			UpdateSHA1_String(HashState, Object->GetPathName());
		}
	}
	FORCEINLINE static void UpdateSHA1_UInt8(FSHA1& HashState, uint8 Number)
	{
		UpdateSHA1_Data(HashState, Number);
		// HashState.Update(&Number, sizeof(Number));
	}
	FORCEINLINE static void UpdateSHA1_Int32(FSHA1& HashState, int32 Number)
	{
		UpdateSHA1_Data(HashState, Number);
		// HashState.Update(reinterpret_cast<const uint8*>(&Number), sizeof(Number));
	}
	FORCEINLINE static void UpdateSHA1_Float(FSHA1& HashState, float Number)
	{
		UpdateSHA1_Data(HashState, Number);
		// UpdateSHA1_Int32(HashState, FMath::RoundToInt(Number));
	}
	FORCEINLINE static void UpdateSHA1_Bool(FSHA1& HashState, bool bBoolean)
	{
		UpdateSHA1_Int32(HashState, static_cast<int32>(bBoolean));
	}
	FORCEINLINE static void UpdateSHA1_Name(FSHA1& HashState, FName Name)
	{
		UpdateSHA1_String(HashState, Name.ToString());
	}
	FORCEINLINE static void UpdateSHA1_String(FSHA1& HashState, const FString& String)
	{
		HashState.UpdateWithString(*String, String.Len());
	}
};

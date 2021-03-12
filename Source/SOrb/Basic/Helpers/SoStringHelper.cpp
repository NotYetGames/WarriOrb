// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoStringHelper.h"

#include "Misc/AES.h"
#include "Misc/SecureHash.h"
#include "Misc/Base64.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString USoStringHelper::RepeatString(const FString& StringToRepeat, int32 RepeatCount, const FString& StringBetween)
{
	if (RepeatCount == 0)
		return "";

	FString ReturnString = StringToRepeat;
	for (int32 i = 1; i < RepeatCount; ++i)
		ReturnString += StringBetween + StringToRepeat;

	return ReturnString;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString USoStringHelper::CapitalizeEachWordFromString(const FString& OriginalString)
{
	if (OriginalString.Len() == 0)
		return OriginalString;

	TArray<FString> WordsArray;
	if (OriginalString.ParseIntoArray(WordsArray, TEXT(" "), true) == 0)
		return OriginalString;

	for (auto& Word : WordsArray)
	{
		if (Word.Len() >= 1)
		{
			Word[0] = FChar::ToUpper(Word[0]);
		}
	}

	return FString::Join(WordsArray, TEXT(" "));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString USoStringHelper::AES256Encrypt(FString InputString, FString Key)
{
	// Check inputs
	if (InputString.IsEmpty()) return "";  //empty string? do nothing
	if (Key.IsEmpty()) return "";

	// To split correctly final result of decryption from trash symbols
	FString SplitSymbol = "EL@$@!";
	InputString.Append(SplitSymbol);

	// We need at least 32 symbols key
	Key = FMD5::HashAnsiString(*Key);
	const TCHAR* KeyTCHAR = Key.GetCharArray().GetData();

	// Calculate blob size and create blob
	uint32 Size =  InputString.Len();
	Size = Size + (FAES::AESBlockSize - (Size % FAES::AESBlockSize));
	uint8* Blob = new uint8[Size];

	// Convert string to bytes and encrypt
	if (StringToBytes(InputString, Blob, Size))
	{
		FAES::EncryptData(Blob, Size, TCHAR_TO_ANSI(KeyTCHAR));
		InputString = FString::FromHexBlob(Blob, Size);

		delete[] Blob;
		return InputString;
	}

	delete[] Blob;
	return TEXT("");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString USoStringHelper::AES256Decrypt(FString InputString, FString Key)
{
	// Check inputs
	if (InputString.IsEmpty()) return "";
	if (Key.IsEmpty()) return "";

	// To split correctly final result of decryption from trash symbols
	FString SplitSymbol = "EL@$@!";

	// We need at least 32 symbols key
	Key = FMD5::HashAnsiString(*Key);
	const TCHAR* KeyTCHAR = Key.GetCharArray().GetData();

	// Calculate blob size and create blob
	uint32 Size =  InputString.Len();
	Size = Size + (FAES::AESBlockSize - (Size % FAES::AESBlockSize));
	uint8* Blob = new uint8[Size];

	// Convert string to bytes and decrypt
	if (FString::ToHexBlob(InputString, Blob, Size))
	{
		FAES::DecryptData(Blob, Size, TCHAR_TO_ANSI(KeyTCHAR));
		InputString = BytesToString(Blob, Size);

		// Split required data from trash
		FString LeftData;
		FString RightData;
		InputString.Split(SplitSymbol, &LeftData, &RightData, ESearchCase::CaseSensitive, ESearchDir::FromStart);
		InputString = LeftData;

		delete[] Blob;
		return InputString;
	}

	delete[] Blob;
	return "";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString USoStringHelper::GetObjectBaseName(const UObject* Object)
{
	if (!IsValid(Object))
		return TEXT("");


	const UPackage* Package = Object->GetOutermost();
	if (!Package)
		return TEXT("");
	
	const FString PathName = Package->GetName();
	const bool bRemovePath = true;
	return FPaths::GetBaseFilename(PathName, bRemovePath);
}
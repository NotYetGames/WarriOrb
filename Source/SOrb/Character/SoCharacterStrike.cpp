// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoCharacterStrike.h"


#include "Basic/Helpers/SoDateTimeHelper.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/Helpers/SoStringHelper.h"

FString USoCharacterStrike::TextNamespace(TEXT("CharacterStrike"));


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoCharacterStrike::USoCharacterStrike()
{
}

#if WITH_EDITOR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterStrike::PreSave(const ITargetPlatform* TargetPlatform)
{
	UpdateLocalizedFields();
	Super::PreSave(TargetPlatform);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterStrike::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterStrike::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedChainEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedChainEvent);
	const int32 ElementIndex = PropertyChangedChainEvent.GetArrayIndex(TEXT("CharacterStrikes"));

	if (CharacterStrikes.IsValidIndex(ElementIndex))
	{
		if (PropertyChangedChainEvent.GetPropertyName() == TEXT("AnimSequenceNew"))
		{
			if (CharacterStrikes[ElementIndex].AnimSequenceNew != nullptr)
				CharacterStrikes[ElementIndex].AnimDuration = USoStaticHelper::GetScaledAnimLength(CharacterStrikes[ElementIndex].AnimSequenceNew);
		}

		if (PropertyChangedChainEvent.GetPropertyName() == TEXT("AnimEndingSequenceLeft"))
		{
			if (CharacterStrikes[ElementIndex].AnimEndingSequenceLeft != nullptr)
				CharacterStrikes[ElementIndex].AnimEndDuration = USoStaticHelper::GetScaledAnimLength(CharacterStrikes[ElementIndex].AnimEndingSequenceLeft);
		}
		if (PropertyChangedChainEvent.GetPropertyName() == TEXT("AnimEndingSequenceRight"))
		{
			if (CharacterStrikes[ElementIndex].AnimEndingSequenceRight != nullptr)
				CharacterStrikes[ElementIndex].AnimEndDuration = USoStaticHelper::GetScaledAnimLength(CharacterStrikes[ElementIndex].AnimEndingSequenceRight);
		}

		if (PropertyChangedChainEvent.GetPropertyName() == TEXT("TrailDelay"))
		{
			for (FSoStrikeEntry& Entry : CharacterStrikes[ElementIndex].StrikeList)
				Entry.TrailDelay = FMath::Min(Entry.TrailDelay, Entry.StrikeDelay);
		}

		if (PropertyChangedChainEvent.GetPropertyName() == TEXT("StrikeDelay"))
		{
			for (FSoStrikeEntry& Entry : CharacterStrikes[ElementIndex].StrikeList)
				Entry.StrikeDelay = FMath::Max(Entry.TrailDelay, Entry.StrikeDelay);
		}
	}
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoCharacterStrike* USoCharacterStrike::GetTemplateFromPath(const FString& Path)
{
	if (Path.Len() == 0)
		return nullptr;

	return Cast<USoCharacterStrike>(StaticLoadObject(USoCharacterStrike::StaticClass(), NULL, *Path));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FSoCharacterStrikeData& USoCharacterStrike::GetStrikeData(int32 Index) const
{
	check(CharacterStrikes.IsValidIndex(Index));
	return CharacterStrikes[Index];
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterStrike::UpdateLocalizedFields()
{
	const FString ObjectName = USoStringHelper::GetObjectBaseName(this);
	StrikeName = FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText(
		*StrikeName.ToString(),
		*TextNamespace,
		*(ObjectName + "_name")
	);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoCharacterStrike::GetCooldownTime() const
{
	return CooldownTime;
}

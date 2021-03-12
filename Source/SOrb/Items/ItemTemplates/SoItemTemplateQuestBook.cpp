// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoItemTemplateQuestBook.h"

#include "Basic/SoGameSingleton.h"
#include "Basic/Helpers/SoStringHelper.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FText& USoItemTemplateQuestBook::GetSubTypeText() const
{
	return USoGameSingleton::GetTextForQuestItemType(ESoQuestItemType::EQIT_Book);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoItemTemplateQuestBook::UpdateLocalizedFields()
{
	const FString ObjectName = USoStringHelper::GetObjectBaseName(this);

	// Update Pages
	for (int32 Index = 0; Index < Pages.Num(); Index++)
	{
		FText& PageText = Pages[Index];

		PageText = FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText(
			*PageText.ToString(),
			*TextNamespace,
			*(ObjectName + "_page_" + FString::FromInt(Index))
		);
	}

	Super::UpdateLocalizedFields();
}

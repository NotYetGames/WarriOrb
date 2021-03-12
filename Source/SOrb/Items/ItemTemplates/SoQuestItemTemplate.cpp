// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoQuestItemTemplate.h"

#include "Basic/SoGameSingleton.h"


#if WITH_EDITOR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoQuestItemTemplate::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateItemType();
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoQuestItemTemplate::PostLoad()
{
	Super::PostLoad();
	UpdateItemType();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FText& USoQuestItemTemplate::GetSubTypeText() const
{
	return USoGameSingleton::GetTextForQuestItemType(QuestItemType);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoQuestItemTemplate::UpdateItemType()
{
	switch (QuestItemType)
	{
		case ESoQuestItemType::EQIT_Key:
		case ESoQuestItemType::EQIT_KeyLikeItem:
			ItemType = ESoItemType::EIT_Key;
			break;

		case ESoQuestItemType::EQIT_MemoryStone:
			ItemType = ESoItemType::EIT_MemoryStone;
			break;

		default:
			ItemType = ESoItemType::EIT_QuestItem;
			break;
	};
}

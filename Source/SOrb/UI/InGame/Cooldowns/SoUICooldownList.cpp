// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUICooldownList.h"

#include "Components/PanelWidget.h"
#include "Components/PanelSlot.h"

#include "SoUICooldownEntry.h"
#include "Character/SoCharacter.h"
#include "Basic/Helpers/SoStaticHelper.h"



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICooldownList::NativeConstruct()
{
	Super::NativeConstruct();

	if (ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
	{
		SoCharacter->OnCooldownStarted().AddDynamic(this, &USoUICooldownList::OnCooldownStarted);
		SoCharacter->OnCooldownEnded().AddDynamic(this, &USoUICooldownList::OnCooldownEnded);
		SoCharacter->OnCooldownBlocksEvent().AddDynamic(this, &USoUICooldownList::OnCooldownBlocksEvent);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICooldownList::NativeDestruct()
{
	if (ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
	{
		SoCharacter->OnCooldownStarted().RemoveDynamic(this, &USoUICooldownList::OnCooldownStarted);
		SoCharacter->OnCooldownBlocksEvent().RemoveDynamic(this, &USoUICooldownList::OnCooldownBlocksEvent);
		SoCharacter->OnCooldownEnded().RemoveDynamic(this, &USoUICooldownList::OnCooldownEnded);
	}

	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICooldownList::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	const TArray<UPanelSlot*>& Slots = Container->GetSlots();
	const int32 OldUsedEntryNum = UsedEntryNum;
	for (int32 i = OldUsedEntryNum - 1; i >= 0; --i)
	{
		if (USoUICooldownEntry* Entry = Cast<USoUICooldownEntry>(Slots[i]->Content))
		{
			if (!Entry->Update())
			{
				// move all data bellow this (maybe not necessary if the list is ordered anyway?), and hide the last entry
				int32 LastIndex = i;
				for (; LastIndex <= OldUsedEntryNum - 2; ++LastIndex)
				{
					USoUICooldownEntry* Current = Cast<USoUICooldownEntry>(Slots[LastIndex]->Content);
					USoUICooldownEntry* Next = Cast<USoUICooldownEntry>(Slots[LastIndex + 1]->Content);
					if (Current != nullptr && Next != nullptr)
						Current->Setup(Cast<USoUICooldownEntry>(Next));
				}

				USoUICooldownEntry* Current = Cast<USoUICooldownEntry>(Slots[LastIndex]->Content);
				if (Current != nullptr)
				{
					Current->ClearAndHide();
					UsedEntryNum -= 1;
				}
			}
		}
	}

	Super::NativeTick(MyGeometry, InDeltaTime);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICooldownList::OnCooldownStarted(int32 Index, float RemainingTime, const UObject* ObjectWithCooldown)
{
	const TArray<UPanelSlot*>& Slots = Container->GetSlots();

	UsedEntryNum += 1;
	for (int32 j = UsedEntryNum - 1; j > Index; --j)
	{
		if (Slots.IsValidIndex(j))
		{
			USoUICooldownEntry* Current = Cast<USoUICooldownEntry>(Slots[j]->Content);
			USoUICooldownEntry* Prev = Cast<USoUICooldownEntry>(Slots[j - 1]->Content);
			if (Current != nullptr && Prev != nullptr)
				Current->Setup(j, Prev->GetObjectWithCooldown(), Prev->GetRemainingTime());
		}
	}

	if (Slots.IsValidIndex(Index))
	{
		USoUICooldownEntry* Current = Cast<USoUICooldownEntry>(Slots[Index]->Content);
		if (Current != nullptr)
			Current->Setup(Index, ObjectWithCooldown, RemainingTime);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICooldownList::OnCooldownEnded(int32 Index, float RemainingTime, const UObject* ObjectWithCooldown)
{
	if (Index >= 0 && Index < UsedEntryNum)
	{
		const TArray<UPanelSlot*>& Slots = Container->GetSlots();
		int32 LastIndex = Index;
		for (; LastIndex <= UsedEntryNum - 2; ++LastIndex)
		{
			USoUICooldownEntry* Current = Cast<USoUICooldownEntry>(Slots[LastIndex]->Content);
			USoUICooldownEntry* Next = Cast<USoUICooldownEntry>(Slots[LastIndex + 1]->Content);
			if (Current != nullptr && Next != nullptr)
				Current->Setup(LastIndex, Next->GetObjectWithCooldown(), Next->GetRemainingTime());
		}

		USoUICooldownEntry* Current = Cast<USoUICooldownEntry>(Slots[LastIndex]->Content);
		if (Current != nullptr)
		{
			Current->ClearAndHide();
			UsedEntryNum -= 1;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICooldownList::OnCooldownBlocksEvent(int32 Index, float RemainingTime, const UObject* ObjectWithCooldown)
{
	const TArray<UPanelSlot*>& Slots = Container->GetSlots();
	if (Slots.IsValidIndex(Index))
	{
		USoUICooldownEntry* Current = Cast<USoUICooldownEntry>(Slots[Index]->Content);
		if (Current != nullptr && Current->GetObjectWithCooldown() == ObjectWithCooldown)
			Current->OnBlockEvent();
	}
}

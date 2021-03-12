// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoUIHelper.h"

#include "Kismet/GameplayStatics.h"
#include "Components/Widget.h"
#include "UObject/UObjectIterator.h"
#include "Engine/Texture.h"
#include "Components/TextBlock.h"

#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"

#include "Character/SoCharacter.h"
#include "Basic/SoAudioManager.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/Helpers/SoMathHelper.h"
#include "Character/SoPlayerController.h"

#include "Items/SoInventoryComponent.h"
#include "Items/ItemTemplates/SoItemTemplateShard.h"
#include "Items/ItemTemplates/SoUsableItemTemplate.h"
#include "Character/SoPlayerCharacterSheet.h"
#include "Basic/Helpers/SoPlatformHelper.h"
#include "Components/Image.h"
#include "Components/HorizontalBoxSlot.h"
#include "Settings/SoGameSettings.h"
#include "General/Commands/SoUICommandImage.h"
#include "UI/InGame/SoUINote.h"

#include "SoLocalization.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoUIHelper, All, All);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoPlayerController* USoUIHelper::GetSoPlayerControllerFromUWidget(const UWidget* Widget)
{
	if (Widget)
		if (APlayerController* PlayerController = Widget->GetOwningPlayer())
			return Cast<ASoPlayerController>(PlayerController);

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FText USoUIHelper::RemapActionsToFText(const TArray<FInputActionKeyMapping>& ByActionMappings)
{
	// Warn about unbinding
	// TODO check if this works
	FFormatOrderedArguments Arguments;

	const int32 NumActions = ByActionMappings.Num();
	FString ActionNames;
	for (int32 Index = 0; Index < NumActions; Index++)
	{
		ActionNames += FSoInputActionName::GetTextForActionName(ByActionMappings[Index].ActionName).ToString();

		// Is not last
		if (Index != NumActions - 1)
		{
			ActionNames += TEXT(", ");
		}
	}

	Arguments.Add(FText::FromString(ActionNames));
	return FText::Format(FText::FromString(TEXT("{0}")), Arguments);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIHelper::AreThereAnyKeyConflicts(const USoGameSettings* UserSettings, FName SelectedActionName, const FInputChord& SelectedKey, FText& OutWarning)
{
	// Key conflicts are allowed but warn the user
	TArray<FInputActionKeyMapping> ByActionMappings;
	if (UserSettings->IsInputChordAlreadyUsed(SelectedActionName, SelectedKey, ESoInputActionCategoryType::GameOrUI, ByActionMappings))
	{
		// Key is valid, reassign to self, why not
		if (ByActionMappings.Num() == 1 && ByActionMappings[0].ActionName == SelectedActionName)
			return true;

		// Warn about unbinding
		const FText ActionNames = RemapActionsToFText(ByActionMappings);
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Key"), FSoInputKey::GetKeyDisplayName(SelectedKey.Key));
		Arguments.Add(TEXT("NumActions"), ByActionMappings.Num());
		Arguments.Add(TEXT("ActionNames"), ActionNames);

		OutWarning = FText::Format(FROM_STRING_TABLE_UI("input_actions_conflicts"), Arguments);
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIHelper::ForceReloadAllUICommandImages()
{
	for (TObjectIterator<USoUICommandImage> Itr; Itr; ++Itr)
	{
		USoUICommandImage* Image = *Itr;
		if (!IsValid(Image))
			continue;

		Image->ForceUpdateImage();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIHelper::OpenEndGameFeedbackBrowserAndExplorer()
{
	// Launch Feedback URL
	static const FString FeedbackURL = TEXT("https://goo.gl/forms/BYUrlPVrWVVDSXSr2");
	USoPlatformHelper::LaunchURL(FeedbackURL, true);

	// Open Saved/ Folder
	static const FString SaveDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
	USoPlatformHelper::ExploreFolder(SaveDir);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIHelper::PreCacheTexture(UTexture* Texture)
{
	if (!Texture)
		return;

	// TODO: figure out best way to precache
	Texture->NeverStream = true;
	//Texture->MipGenSettings = TMGS_NoMipmaps;
	// Texture->LODGroup = TEXTUREGROUP_UI;
	// Texture->UpdateCachedLODBias();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIHelper::PreCacheTexture2D(UTexture2D* Texture)
{
	if (!Texture)
		return;

	PreCacheTexture(Texture);
	//const int32 AllowedMips = Texture->GetNumMipsAllowed(false);
	//Texture->StreamIn(AllowedMips, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIHelper::PreCacheImage(UImage* Image)
{
	if (!Image)
		return;

	PreCacheTexture(Cast<UTexture>(Image->Brush.GetResourceObject()));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIHelper::CopyHorizontalBoxSlotToAnother(const UHorizontalBoxSlot* From, UHorizontalBoxSlot* To)
{
	if (!From || !To)
		return false;

	To->Size = From->Size;
	To->Padding = From->Padding;
	To->HorizontalAlignment = From->HorizontalAlignment;
	To->VerticalAlignment = From->VerticalAlignment;
	To->SynchronizeProperties();

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIHelper::SubscribeOnHealthChanged(UObject* WorldContextObject,
										   const FSoUINotifyTwoFloatSingle& OnHealthChanged,
										   bool bSubscribe)
{
	ASoCharacter* SoChar = USoStaticHelper::GetPlayerCharacterAsSoCharacter(WorldContextObject);
	if (!SoChar)
		return;

	FSoUINotifyTwoFloat& Event = SoChar->GetPlayerCharacterSheet()->OnHealthChanged;
	if (bSubscribe)
	{
		if (!Event.Contains(OnHealthChanged))
			Event.Add(OnHealthChanged);
		else
			UE_LOG(LogSoUIHelper, Warning, TEXT("SubscribeOnHealthChanged: Tried to subscribe but Event already exists"));
	}
	else
	{
		if (Event.Contains(OnHealthChanged))
			Event.Remove(OnHealthChanged);
		else
			UE_LOG(LogSoUIHelper, Warning, TEXT("SubscribeOnHealthChanged: Tried to unsubscribe but Event does not exist"));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIHelper::SubscribeOnDmgApplied(UObject* WorldContextObject,
										const FSoUINotifyDmgSingle& OnDmgApplied,
										bool bSubscribe)
{
	ASoCharacter* SoChar = USoStaticHelper::GetPlayerCharacterAsSoCharacter(WorldContextObject);
	if (!SoChar)
		return;

	FSoUINotifyDmg& Event = SoChar->GetPlayerCharacterSheet()->OnDmgApplied;
	if (bSubscribe)
	{
		if (!Event.Contains(OnDmgApplied))
			Event.Add(OnDmgApplied);
		else
			UE_LOG(LogSoUIHelper, Warning, TEXT("SubscribeOnHealthChanged: Tried to subscribe but Event already exists"));
	}
	else
	{
		if (Event.Contains(OnDmgApplied))
			Event.Remove(OnDmgApplied);
		else
			UE_LOG(LogSoUIHelper, Warning, TEXT("SubscribeOnHealthChanged: Tried to unsubscribe but Event does not exist"));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIHelper::SubscribeOnBonusHealthChanged(UObject* WorldContextObject,
												const FSoUINotifyDmgSingle& OnBonusHealthChanged,
												bool bSubscribe)
{
	ASoCharacter* SoChar = USoStaticHelper::GetPlayerCharacterAsSoCharacter(WorldContextObject);
	if (!SoChar)
		return;

	FSoUINotifyDmg& Event = SoChar->GetPlayerCharacterSheet()->OnBonusHealthChanged;
	if (bSubscribe)
	{
		if (!Event.Contains(OnBonusHealthChanged))
			Event.Add(OnBonusHealthChanged);
		else
			UE_LOG(LogSoUIHelper, Warning, TEXT("SubscribeOnHealthChanged: Tried to subscribe but Event already exists"));
	}
	else
	{
		if (Event.Contains(OnBonusHealthChanged))
			Event.Remove(OnBonusHealthChanged);
		else
			UE_LOG(LogSoUIHelper, Warning, TEXT("SubscribeOnHealthChanged: Tried to unsubscribe but Event does not exist"));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIHelper::Equip(UObject* WorldContextObject, int32 SourceInventoryIndex, ESoItemSlot TargetSlot)
{
	ASoCharacter* SoChar = USoStaticHelper::GetPlayerCharacterAsSoCharacter(WorldContextObject);
	if (!SoChar)
		return;

	SoChar->GetPlayerCharacterSheet()->Equip(SourceInventoryIndex, TargetSlot);

	UFMODEvent* EquipSFX = nullptr;
	switch (TargetSlot)
	{
		case ESoItemSlot::EIS_ShardDefensive0:
		case ESoItemSlot::EIS_ShardDefensive1:
		case ESoItemSlot::EIS_ShardOffensive0:
		case ESoItemSlot::EIS_ShardOffensive1:
		case ESoItemSlot::EIS_ShardSpecial:
			EquipSFX = SoChar->SFXFClothEquip;
			break;

		case ESoItemSlot::EIS_Item0:
		case ESoItemSlot::EIS_Item1:
		{
			USoUsableItemTemplate* Template = Cast<USoUsableItemTemplate>(SoChar->GetPlayerCharacterSheet()->GetEquippedItem(TargetSlot).Template);
			if (Template != nullptr && Template->UsableType == ESoUsableItemType::EUIT_Potion)
				EquipSFX = SoChar->SFXPotionEquip;
			else
				EquipSFX = SoChar->SFXUsableEquip;
		}
		break;

		case ESoItemSlot::EIS_Weapon0:
		case ESoItemSlot::EIS_Weapon1:
			EquipSFX = SoChar->SFXWeaponEquip;
			break;
		default:
			break;
	}

	if (EquipSFX != nullptr)
		USoAudioManager::PlaySoundAtLocation(SoChar, EquipSFX, {});
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIHelper::EquipFromSlot(UObject* WorldContextObject, int32 SourceInventoryIndex)
{
	ASoCharacter* SoChar = USoStaticHelper::GetPlayerCharacterAsSoCharacter(WorldContextObject);
	if (!SoChar)
		return;

	if (SoChar->GetInventory()->IsValidIndex(SourceInventoryIndex))
	{
		const FSoItem& Item = SoChar->GetInventory()->GetItem(SourceInventoryIndex);
		UFMODEvent* EquipSFX = nullptr;

		switch (Item.Template->GetItemType())
		{
		case ESoItemType::EIT_Shard:
			EquipSFX = SoChar->SFXFClothEquip;
			break;

		case ESoItemType::EIT_Weapon:
			EquipSFX = SoChar->SFXWeaponEquip;
			break;

		case ESoItemType::EIT_UsableItem:
			if (Cast<USoUsableItemTemplate>(Item.Template)->UsableType == ESoUsableItemType::EUIT_Potion)
				EquipSFX = SoChar->SFXPotionEquip;
			else
				EquipSFX = SoChar->SFXUsableEquip;
			break;

		default:
			break;
		}

		if (EquipSFX != nullptr)
			USoAudioManager::PlaySoundAtLocation(SoChar, EquipSFX, {});
	}
	SoChar->GetPlayerCharacterSheet()->EquipFromSlot(SourceInventoryIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIHelper::Unequip(UObject* WorldContextObject, ESoItemSlot TargetSlot)
{
	ASoCharacter* SoChar = USoStaticHelper::GetPlayerCharacterAsSoCharacter(WorldContextObject);
	if (!SoChar)
		return;

	SoChar->GetPlayerCharacterSheet()->Unequip(TargetSlot);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIHelper::CanPlayerEquipToSlot(UObject* WorldContextObject, ESoItemSlot TargetSlot)
{
	ASoCharacter* SoChar = USoStaticHelper::GetPlayerCharacterAsSoCharacter(WorldContextObject);
	return SoChar->CanEquipToSlot(TargetSlot);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIHelper::CanPlayerEquipItem(UObject* WorldContextObject, USoItemTemplate* Template)
{
	if (Template == nullptr || !Template->IsEquipable())
		return false;

	return USoUIHelper::CanPlayerEquipToSlot(WorldContextObject, Template->GetDefaultSlot());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIHelper::DrinkPotionFromInventory(UObject* WorldContextObject, int32 SourceInventoryIndex)
{
	ASoCharacter* SoChar = USoStaticHelper::GetPlayerCharacterAsSoCharacter(WorldContextObject);
	if (SoChar != nullptr && SoChar->GetPlayerCharacterSheet() != nullptr)
		SoChar->GetPlayerCharacterSheet()->DrinkPotionFromInventory(SourceInventoryIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIHelper::SwapSlots(UObject* WorldContextObject, ESoItemSlot SourceSlot, ESoItemSlot TargetSlot)
{
	ASoCharacter* SoChar = USoStaticHelper::GetPlayerCharacterAsSoCharacter(WorldContextObject);
	if (SoChar != nullptr)
		SoChar->GetPlayerCharacterSheet()->SwapSlots(SourceSlot, TargetSlot);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FSoItem& USoUIHelper::GetEquippedItem(UObject* WorldContextObject, ESoItemSlot ItemSlot)
{
	ASoCharacter* SoChar = USoStaticHelper::GetPlayerCharacterAsSoCharacter(WorldContextObject);
	return SoChar->GetPlayerCharacterSheet()->GetEquippedItem(ItemSlot);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIHelper::HasItem(UObject* WorldContextObject, USoItemTemplate* Item)
{
	if (WorldContextObject != nullptr)
	{
		ASoCharacter* SoChar = USoStaticHelper::GetPlayerCharacterAsSoCharacter(WorldContextObject);
		return SoChar->GetInventory()->HasItem(Item) || SoChar->GetPlayerCharacterSheet()->IsItemEquipped(Item);
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoItemTemplateShard* USoUIHelper::GetShardTemplateFromEquipped(UObject* WorldContextObject, ESoItemSlot ItemSlot)
{
	ASoCharacter* SoChar = USoStaticHelper::GetPlayerCharacterAsSoCharacter(WorldContextObject);
	FSoItem& Item = SoChar->GetPlayerCharacterSheet()->GetEquippedItem(ItemSlot);
	return Cast<USoItemTemplateShard>(Item.Template);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoItemTemplateShard* USoUIHelper::GetShardTemplateFromItems(const TArray<USoItemTemplate*>& Items, ESoItemSlot ItemSlot)
{
	// TODO make the world state metadata entry to be a map? O(1) instead of O(20) does it make sense?
	for (USoItemTemplate* Template : Items)
		if (USoItemTemplateShard* ShardTemplate = Cast<USoItemTemplateShard>(Template))
			if (ShardTemplate->GetDefaultSlot() == ItemSlot)
				return ShardTemplate;

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIHelper::HasItemEquippedAboveSlot(UObject* WorldContextObject, ESoItemSlot ItemSlot)
{
	// DO we need this?
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIHelper::SubscribeOnEquipmentSlotChanged(UObject* WorldContextObject,
												  const FSoEquipmentSlotChangedSingle& OnEquipmentSlotChanged,
												  bool bSubscribe)
{
	ASoCharacter* SoChar = USoStaticHelper::GetPlayerCharacterAsSoCharacter(WorldContextObject);
	if (!SoChar)
		return;

	if (bSubscribe)
		SoChar->GetPlayerCharacterSheet()->OnSlotChanged.Add(OnEquipmentSlotChanged);
	else
		SoChar->GetPlayerCharacterSheet()->OnSlotChanged.Remove(OnEquipmentSlotChanged);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIHelper::SubscribeOnRecalculateAttributes(UObject* WorldContextObject,
												   const FSoUINotifySingle& OnRecalculateAttributes,
												   bool bSubscribe)
{
	ASoCharacter* SoChar = USoStaticHelper::GetPlayerCharacterAsSoCharacter(WorldContextObject);
	if (!SoChar)
		return;

	if (bSubscribe)
		SoChar->GetPlayerCharacterSheet()->OnRecalculateAttributes.Add(OnRecalculateAttributes);
	else
		SoChar->GetPlayerCharacterSheet()->OnRecalculateAttributes.Remove(OnRecalculateAttributes);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoUIHelper::NavigateInMatrix(ESoUICommand Command, int32 StartIndex, int32 ColNum, int32 ItemNum)
{
	Command = TryTranslateMenuCommandDirectionToGame(Command);
	const int32 RowNum = ItemNum / ColNum + 1;

	if (ItemNum < 2 || RowNum < 1 || ColNum < 1)
		return 0;


	int32 ColIndex = StartIndex % ColNum;
	int32 RowIndex = StartIndex / ColNum;

	// col num in the selected row, used if we step inside the row, can be < in the last row
	const int32 ColNumInRow = (RowIndex == RowNum - 1) ? ItemNum - (RowNum - 1) * ColNum : ColNum;
	// row num in selected col, used if we step inside the col, can be < if the last line doesn't have item at this row;
	const int32 RowNumInCol = (ColIndex < ItemNum % ColNum) ? RowNum : RowNum - 1;

	switch (Command)
	{
		case ESoUICommand::EUC_Down:
			RowIndex = (RowIndex + 1) % RowNumInCol;
			break;

		case ESoUICommand::EUC_Up:
			RowIndex = USoMathHelper::WrapIndexAround(RowIndex - 1, RowNumInCol);
			break;

		case ESoUICommand::EUC_Left:
			ColIndex = USoMathHelper::WrapIndexAround(ColIndex - 1, ColNumInRow);
			break;

		case ESoUICommand::EUC_Right:
			ColIndex = (ColIndex + 1) % ColNumInRow;
			break;

		default:
			break;
	}

	return RowIndex * ColNum + ColIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoInventoryComponent* USoUIHelper::GetPlayerInventory(UObject* WorldContextObject)
{
	ASoCharacter* SoChar = USoStaticHelper::GetPlayerCharacterAsSoCharacter(WorldContextObject);
	if (SoChar != nullptr)
		return SoChar->GetInventory();

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoItemType USoUIHelper::GetTypeFromSlot(ESoItemSlot Slot)
{
	switch (Slot)
	{
		case ESoItemSlot::EIS_ShardDefensive0:
		case ESoItemSlot::EIS_ShardDefensive1:
		case ESoItemSlot::EIS_ShardOffensive0:
		case ESoItemSlot::EIS_ShardOffensive1:
		case ESoItemSlot::EIS_ShardSpecial:
			return ESoItemType::EIT_Shard;

		case ESoItemSlot::EIS_Item0:
		case ESoItemSlot::EIS_Item1:
			return ESoItemType::EIT_UsableItem;

		case ESoItemSlot::EIS_Weapon0:
		case ESoItemSlot::EIS_Weapon1:
			return ESoItemType::EIT_Weapon;

		default:
			return ESoItemType::EIT_MAX;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoUIHelper::GetSelectedIndexAfterNavigation(ESoUICommand Command, int32 StartIndex, const TArray<ESoItemSlot>& SlotArray)
{
	// translate MainMenu commands to Left/Right/Up/Down
	Command = TryTranslateMenuCommandDirectionToGame(Command);

	if (Command > ESoUICommand::EUC_Down)
		return StartIndex;

	verify(static_cast<int32>(Command) < 4);

	// NextSlot = Slots[SourceSlot][left | right | up | down]
	static constexpr ESoItemSlot Slots[static_cast<int32>(ESoItemSlot::EIS_Max)+2][4] =
	{
		/* EIS_ShardDefensive0	*/ { ESoItemSlot::EIS_ShardDefensive1,	ESoItemSlot::EIS_ShardDefensive1,	ESoItemSlot::EIS_Weapon1,			ESoItemSlot::EIS_ShardSpecial },
		/* EIS_ShardDefensive1	*/ { ESoItemSlot::EIS_ShardDefensive0,	ESoItemSlot::EIS_ShardDefensive0,	ESoItemSlot::EIS_Item1,				ESoItemSlot::EIS_ShardSpecial },
		/* EIS_ShardOffensive0	*/ { ESoItemSlot::EIS_ShardOffensive1,	ESoItemSlot::EIS_ShardOffensive1,	ESoItemSlot::EIS_ShardSpecial,		ESoItemSlot::EIS_Weapon0 },
		/* EIS_ShardOffensive1	*/ { ESoItemSlot::EIS_ShardOffensive0,	ESoItemSlot::EIS_ShardOffensive0,	ESoItemSlot::EIS_ShardSpecial,		ESoItemSlot::EIS_Item0 },
		/* EIS_ShardSpecial		*/ { ESoItemSlot::EIS_ShardDefensive0,	ESoItemSlot::EIS_ShardDefensive1,	ESoItemSlot::EIS_ShardDefensive0,	ESoItemSlot::EIS_ShardOffensive0 },

		/* EIS_Item0			*/ { ESoItemSlot::EIS_Weapon0,		ESoItemSlot::EIS_Weapon0,	ESoItemSlot::EIS_ShardOffensive1,	ESoItemSlot::EIS_Item1 },
		/* EIS_Item1			*/ { ESoItemSlot::EIS_Weapon1,		ESoItemSlot::EIS_Weapon1,	ESoItemSlot::EIS_Item0,				ESoItemSlot::EIS_ShardDefensive1 },

		/* EIS_Weapon0			*/ { ESoItemSlot::EIS_Item0,		ESoItemSlot::EIS_Item0,		ESoItemSlot::EIS_ShardOffensive0,	ESoItemSlot::EIS_Weapon1 },
		/* EIS_Weapon1			*/ { ESoItemSlot::EIS_Item1,		ESoItemSlot::EIS_Item1,		ESoItemSlot::EIS_Weapon0,			ESoItemSlot::EIS_ShardDefensive0 },
	};

	const int32 Index = static_cast<int32>(SlotArray[StartIndex]);

	auto FindIndex = [&SlotArray, StartIndex](ESoItemSlot Slot)
	{
		for (int32 i = 0; i < SlotArray.Num(); ++i)
			if (SlotArray[i] == Slot)
				return i;

		return StartIndex;
	};

	return FindIndex(Slots[Index][static_cast<int32>(Command)]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIHelper::AddItemsToCharacter(UObject* WorldContextObject, const TArray<FSoItem>& ItemsToAdd)
{
	ASoCharacter* SoChar = USoStaticHelper::GetPlayerCharacterAsSoCharacter(WorldContextObject);
	for (const FSoItem& Item : ItemsToAdd)
		SoChar->AddItem(Item);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIHelper::PredicateSortNoteAddedTime(const USoUINote& A, const USoUINote& B)
{
	if (ASoCharacter* SoChar = USoStaticHelper::GetPlayerCharacterAsSoCharacter(&A))
	{
		const float TimeA = IDlgDialogueParticipant::Execute_GetFloatValue(SoChar, A.ID);
		const float TimeB = IDlgDialogueParticipant::Execute_GetFloatValue(SoChar, B.ID);

		return TimeA < TimeB;
	}

	return false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TArray<USoUINote*> USoUIHelper::OrderNotes(UObject* WorldContextObject, const TArray<USoUINote*>& NotesToOrder)
{
	TArray<USoUINote*> OrderedNotes;
	TArray<USoUINote*> HiddenNotes;

	if (ASoCharacter* SoChar = USoStaticHelper::GetPlayerCharacterAsSoCharacter(WorldContextObject))
		for (USoUINote* Note : NotesToOrder)
		{
			if (IDlgDialogueParticipant::Execute_GetIntValue(SoChar, Note->ID) > 0)
				OrderedNotes.Add(Note);
			else
				HiddenNotes.Add(Note);
		}

	OrderedNotes.Sort(USoUIHelper::PredicateSortNoteAddedTime);
	OrderedNotes.Append(HiddenNotes);

	return OrderedNotes;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector2D USoUIHelper::MeasureWrappedString(const FSlateFontInfo& FontInfo, const FString& Text, float WrapWidth)
{
	// CODE FROM FORUM
	// https://forums.unrealengine.com/development-discussion/blueprint-visual-scripting/1656855-how-to-measure-rendered-size-of-a-string

	FVector2D Size(0.0f, 0.0f);

	// Everything we do here duplicates what FSlateFontMeasure::MeasureStringInternal()
	// already does. Sadly, the existing method is private and there's no exposed method
	// that happens to call it in a way we can work with...

	// Scan the entire string, keeping track of where lines begin and checking every time
	// a whitespace is encountered how long the line would be when wrapped there.
	// We could do this in a more fancy way with binary search and by guessing lengths,
	// but it's typically done just one when new text is displayed, so this is good enough.
	{
		TSharedRef<FSlateFontMeasure> FontMeasureService = (FSlateApplication::Get().GetRenderer()->GetFontMeasureService());
		int32 LineCount = 1;
		bool bLastWasWhitespace = true;

		bool bFoundLineStartIndex = false;
		int32 LineStartIndex = 0;

		int32 LastGoodWrapIndex = -1;
		float LastGoodWrapWidth = 0.0f;

		// Scanning loop, steps through the string character by character
		int32 TextLength = Text.Len();
		for (int32 Index = 0; Index < TextLength; ++Index)
		{

			// Check if the current character is a whitespace character (and thus, the line
			// can be broken at this point)
			TCHAR Character = Text[Index];
			bool bIsWhitespace = ((Character == TEXT(' ')) || (Character == TEXT('\t')) || (Character == TEXT('\n')));

			// If we have a line start index (meaning there was a non-whitespace character),
			// do the line break checking
			if (bFoundLineStartIndex)
			{
				// Don't re-check line breaks on consecutive whitespaces
				if (bIsWhitespace && bLastWasWhitespace)
				{
					continue;
				}
				bLastWasWhitespace = bIsWhitespace;

				// If this is no whitespace, we can't wrap here, so continue scanning
				if (!bIsWhitespace)
				{
					continue;
				}

				// Measure the line up until the whitespace we just encountered
				FVector2D PotentialLineSize = FontMeasureService->Measure(Text, LineStartIndex, Index - 1, FontInfo, false);

			 // If it still fits in the line, remember this as the most recent good wrap ppoint
				if (PotentialLineSize.X < WrapWidth)
				{
					LastGoodWrapIndex = Index;
					LastGoodWrapWidth = PotentialLineSize.X;
				}
				else
				{
					++LineCount;

					if (LastGoodWrapIndex == -1)
					{
						// First whitespace and it's already too long...
						Size.X = FMath::Max(Size.X, PotentialLineSize.X);
					}
					else
					{
						// Phew... we have a good wrapping position remembered
						Size.X = FMath::Max(Size.X, LastGoodWrapWidth);
					}

					// Reset all trackers to scan for a new line from here
					LastGoodWrapIndex = -1;
					LineStartIndex = Index;
					bFoundLineStartIndex = false;
				 }

			}
			else if (!bIsWhitespace)
			{
				bFoundLineStartIndex = true; // The first non-whitespace character marks the line start
				LineStartIndex = Index;
			}

		} // for

		// If there are characters remaining on the last line, measure them, too
		// (we also know it doesn't end in a space/newline because otherwise this
		// property would have a value of false, thus this final check is really basic)
		if (bFoundLineStartIndex)
		{
			FVector2D finalLineSize = FontMeasureService->Measure(Text, LineStartIndex, TextLength - 1, FontInfo, false);
			Size.X = FMath::Max(Size.X, finalLineSize.X);
		}

		Size.Y = (static_cast<float>(FontMeasureService->GetMaxCharacterHeight(FontInfo)) * LineCount);
	}

	return Size;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIHelper::FixTextSize(UTextBlock* TextBlock, float WrapWidth, int32 DefaultFontSize, float MaxAllowedHeight, int32 MinFontSize)
{
	if (TextBlock == nullptr)
		return;

	auto SetFontSize = [TextBlock](float Size)
	{
		FSlateFontInfo FontInfo = TextBlock->Font;
		FontInfo.Size = Size;
		TextBlock->SetFont(FontInfo);
	};

	SetFontSize(DefaultFontSize);

	float ActualHeight = MeasureWrappedString(TextBlock->Font, TextBlock->GetText().ToString(), WrapWidth).Y;
	for (int32 FontSize = DefaultFontSize - 1; ActualHeight > MaxAllowedHeight && FontSize >= MinFontSize; --FontSize)
	{
		SetFontSize(FontSize);
		ActualHeight = MeasureWrappedString(TextBlock->Font, TextBlock->GetText().ToString(), WrapWidth).Y;
	}
}

FLinearColor USoUIHelper::GetFrameMSDisplayColor(float FrameTimeMS)
{
	if (!GEngine)
		return FLinearColor::White;

	return FLinearColor(GEngine->GetFrameTimeDisplayColor(FrameTimeMS));
}

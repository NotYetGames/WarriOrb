// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Items/SoItem.h"
#include "UI/General/SoUITypes.h"

#include "SoUIHelper.generated.h"

class UHorizontalBoxSlot;
class ASoPlayerController;
class UWidget;
class UTexture2D;
class UImage;
class UTexture;
class USoGameSettings;
class USoUINote;
class USoItemTemplate;
class USoItemTemplateShard;
class USoInventoryComponent;
class UTextBlock;
struct FInputChord;
struct FInputActionKeyMapping;


/**
 *  Interface between the player character and UMG
 *  Had to be created because when the UI knew the
 *  ASoCharacter/USoPlayerCharacterSheet that lead to
 *  "REINST" errors
 */
UCLASS()
class SORB_API USoUIHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	//
	// Helper
	//
	static ASoPlayerController* GetSoPlayerControllerFromUWidget(const UWidget* Widget);

	// Input settings helper
	static FText RemapActionsToFText(const TArray<FInputActionKeyMapping>& ByActionMappings);
	static bool AreThereAnyKeyConflicts(const USoGameSettings* UserSettings, FName SelectedActionName, const FInputChord& SelectedKey, FText& OutWarning);

	// Reloads the image of all the command images
	static void ForceReloadAllUICommandImages();


	//UFUNCTION(BlueprintPure, Category = "UI", meta = (WorldContext = "WorldContextObject"))
	//static bool AreUIInputsPressed(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "UI")
	static void OpenEndGameFeedbackBrowserAndExplorer();

	// Load texture into memory so that we do not see any blurring images.
	UFUNCTION(BlueprintCallable, Category = "Cache")
	static void PreCacheTexture(UTexture* Texture);

	UFUNCTION(BlueprintCallable, Category = "Cache")
	static void PreCacheTexture2D(UTexture2D* Texture);

	UFUNCTION(BlueprintCallable, Category = "Cache")
	static void PreCacheImage(UImage* Texture);

	UFUNCTION(BlueprintCallable)
	static bool CopyHorizontalBoxSlotToAnother(const UHorizontalBoxSlot* From, UHorizontalBoxSlot* To);

	// Tries to convert a Menu<Command> to a Game Command
	// If no match is found just returns the original input Command
	UFUNCTION(BlueprintPure, Category = "UI")
	static FORCEINLINE ESoUICommand TryTranslateMenuCommandDirectionToGame(ESoUICommand Command)
	{
		switch (Command)
		{
		case ESoUICommand::EUC_MainMenuLeft:
			return ESoUICommand::EUC_Left;
		case ESoUICommand::EUC_MainMenuRight:
			return ESoUICommand::EUC_Right;
		case ESoUICommand::EUC_MainMenuDown:
			return ESoUICommand::EUC_Down;
		case ESoUICommand::EUC_MainMenuUp:
			return ESoUICommand::EUC_Up;
			//case ESoUICommand::EUC_MainMenuBack:
				//return ESoUICommand::EUC_ActionBack;
		default:
			break;
		}

		return Command;
	}

	//
	// Subscribe
	//
	UFUNCTION(BlueprintCallable, Category = "Character", meta = (WorldContext = "WorldContextObject"))
	static void SubscribeOnHealthChanged(UObject* WorldContextObject,
										 const FSoUINotifyTwoFloatSingle& OnHealthChanged,
										 bool bSubscribe = true);

	UFUNCTION(BlueprintCallable, Category = "Character", meta = (WorldContext = "WorldContextObject"))
	static void SubscribeOnDmgApplied(UObject* WorldContextObject,
									  const FSoUINotifyDmgSingle& OnDmgApplied,
									  bool bSubscribe = true);

	UFUNCTION(BlueprintCallable, Category = "Character", meta = (WorldContext = "WorldContextObject"))
	static void SubscribeOnBonusHealthChanged(UObject* WorldContextObject,
											  const FSoUINotifyDmgSingle& OnBonusHealthChanged,
											  bool bSubscribe = true);

	//
	// CharacterSheet
	//

	UFUNCTION(BlueprintPure, Category = "PlayerEquipment")
	static int32 GetSelectedIndexAfterNavigation(ESoUICommand Command, int32 StartIndex, const TArray<ESoItemSlot>& SlotArray);

	UFUNCTION(BlueprintPure, Category = "PlayerEquipment")
	static ESoItemType GetTypeFromSlot(ESoItemSlot Slot);

	// Equip an item from the inventory
	UFUNCTION(BlueprintCallable, Category = "PlayerEquipment", meta = (WorldContext = "WorldContextObject"))
	static void Equip(UObject* WorldContextObject, int32 SourceInventoryIndex, ESoItemSlot TargetSlot);

	// Equip an item from the inventory
	UFUNCTION(BlueprintCallable, Category = "PlayerEquipment", meta = (WorldContext = "WorldContextObject"))
	static void EquipFromSlot(UObject* WorldContextObject, int32 SourceInventoryIndex);

	UFUNCTION(BlueprintCallable, Category = "PlayerEquipment", meta = (WorldContext = "WorldContextObject"))
	static void Unequip(UObject* WorldContextObject, ESoItemSlot TargetSlot);

	UFUNCTION(BlueprintPure, Category = "PlayerEquipment", meta = (WorldContext = "WorldContextObject"))
	static bool CanPlayerEquipToSlot(UObject* WorldContextObject, ESoItemSlot TargetSlot);

	UFUNCTION(BlueprintPure, Category = "PlayerEquipment", meta = (WorldContext = "WorldContextObject"))
	static bool CanPlayerEquipItem(UObject* WorldContextObject, USoItemTemplate* Template);

	UFUNCTION(BlueprintCallable, Category = "PlayerEquipment", meta = (WorldContext = "WorldContextObject"))
	static void DrinkPotionFromInventory(UObject* WorldContextObject, int32 SourceInventoryIndex);


	// move an item from a character sheet slot to another
	// TargetSlot can be empty, SourceSlot can not
	UFUNCTION(BlueprintCallable, Category = "PlayerEquipment", meta = (WorldContext = "WorldContextObject"))
	static void SwapSlots(UObject* WorldContextObject, ESoItemSlot SourceSlot, ESoItemSlot TargetSlot);

	UFUNCTION(BlueprintCallable, Category = "PlayerEquipment", meta = (WorldContext = "WorldContextObject"))
	static const FSoItem& GetEquippedItem(UObject* WorldContextObject, ESoItemSlot ItemSlot);

	UFUNCTION(BlueprintPure, Category = "PlayerEquipment", meta = (WorldContext = "WorldContextObject"))
	static bool HasItem(UObject* WorldContextObject, USoItemTemplate* Item);

	UFUNCTION(BlueprintPure, Category = "PlayerEquipment", meta = (WorldContext = "WorldContextObject"))
	static USoItemTemplateShard* GetShardTemplateFromEquipped(UObject* WorldContextObject, ESoItemSlot ItemSlot);

	// Same as GetShardTemplateFromEquipped but this gets the cloth template from a list of items
	UFUNCTION(BlueprintPure, Category = "PlayerEquipment")
	static USoItemTemplateShard* GetShardTemplateFromItems(const TArray<USoItemTemplate*>& Items, ESoItemSlot ItemSlot);

	UFUNCTION(BlueprintCallable, Category = "PlayerEquipment", meta = (WorldContext = "WorldContextObject"))
	static void SubscribeOnEquipmentSlotChanged(UObject* WorldContextObject,
												const FSoEquipmentSlotChangedSingle& OnEquipmentSlotChanged,
												bool bSubscribe = true);

	UFUNCTION(BlueprintPure, Category = "PlayerEquipment", meta = (WorldContext = "WorldContextObject"))
	static bool HasItemEquippedAboveSlot(UObject* WorldContextObject, ESoItemSlot ItemSlot);

	UFUNCTION(BlueprintCallable, Category = "PlayerEquipment", meta = (WorldContext = "WorldContextObject"))
	static void SubscribeOnRecalculateAttributes(UObject* WorldContextObject,
												 const FSoUINotifySingle& OnRecalculateAttributes,
												 bool bSubscribe = true);


	UFUNCTION(BlueprintCallable, Category = "PlayerPresets")
	static int32 NavigateInMatrix(ESoUICommand Command, int32 StartIndex, int32 ColNum, int32 ItemNum);


	//
	// Inventory
	//
	UFUNCTION(BlueprintPure, Category = "PlayerEquipment", meta = (WorldContext = "WorldContextObject"))
	static USoInventoryComponent* GetPlayerInventory(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "PlayerEquipment", meta = (WorldContext = "WorldContextObject"))
	static void AddItemsToCharacter(UObject* WorldContextObject, const TArray<FSoItem>& ItemsToAdd);



	static bool PredicateSortNoteAddedTime(const USoUINote& A, const USoUINote& B);

	UFUNCTION(BlueprintCallable, Category = "Notes", meta = (WorldContext = "WorldContextObject"))
	static TArray<USoUINote*> OrderNotes(UObject* WorldContextObject, const TArray<USoUINote*>& NotesToOrder);

	UFUNCTION(BlueprintPure, Category = "FontHelper", meta = (WorldContext = "WorldContextObject"))
	static FVector2D MeasureWrappedString(const FSlateFontInfo& FontInfo, const FString& Text, float WrapWidth);

	UFUNCTION(BlueprintCallable, Category = "FontHelper", meta = (WorldContext = "WorldContextObject"))
	static void FixTextSize(UTextBlock* TextBlock, float WrapWidth, int32 DefaultFontSize, float MaxAllowedHeight, int32 MinFontSize);

	//
	// Color
	//

	// Gets the color for the specified frame time in MS
	UFUNCTION(BlueprintPure, Category = ">UI|Color")
	static FLinearColor GetFrameMSDisplayColor(float FrameTimeMS);

	// Gets the color for the specifeid frame FPS
	UFUNCTION(BlueprintPure, Category = ">UI|Color")
	static FLinearColor GetFrameFPSDisplayColor(float FrameFPS)
	{
		return GetFrameMSDisplayColor(1000.f / FrameFPS);
	}

};

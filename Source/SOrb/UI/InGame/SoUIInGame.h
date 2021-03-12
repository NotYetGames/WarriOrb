// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/General/SoUITypes.h"

#include "SoUIInGame.generated.h"

class USoUICharacterPanels;
class USoInGameUIActivity;
class USoUIDialoguePanel;


/**
 *
 */
UCLASS()
class SORB_API USoUIInGame : public UUserWidget
{
	GENERATED_BODY()

protected:
	UFUNCTION(BlueprintCallable, Category = UI)
	UUserWidget* GetNotUsedWidget(TSubclassOf<UUserWidget> Class);

protected:

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICharacterPanels* CharacterPanels = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoInGameUIActivity* ItemContainer = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UUserWidget* Book = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIDialoguePanel* DialoguePanel = nullptr;

	bool bUIVisible = true;


	UPROPERTY()
	TMap<TSubclassOf<UUserWidget>, FSoUIWidgetArray> WidgetStorage;
};

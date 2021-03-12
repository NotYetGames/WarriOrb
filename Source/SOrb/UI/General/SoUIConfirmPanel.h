// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "UI/General/SoUITypes.h"

#include "SoUIConfirmPanel.generated.h"

class USoUICommandTooltipArray;
class USoUIButtonArray;
class USoUIButtonImageArray;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSoConfirmPanelQuestionAnsweredEvent);


/**
 * ConfirmPanel base class so that we are sure the UserWidget implements the ISoUIEventHandler
 */
UCLASS(HideCategories = (Navigation), AutoCollapseCategories = (Performance, Clipping, Interaction, Input))
class SORB_API USoUIConfirmPanel : public UUserWidget, public ISoUIEventHandler
{
	GENERATED_BODY()

public:

	//
	// UUserWidget Interface
	//
	void SynchronizeProperties() override;

	void NativeConstruct() override
	{
		Super::NativeConstruct();
		SetVisibility(ESlateVisibility::Collapsed);
		bOpened = false;
	}

	//
	// ISoUIEventHandler Interface
	//
	bool OnUICommand_Implementation(ESoUICommand Command) override { return false; };
	void Open_Implementation(bool bOpen) override;
	bool IsOpened_Implementation() const override { return bOpened; }
	bool CanBeInterrupted_Implementation() const override { return true; }

	//
	// Own Methods
	//

	UFUNCTION(BlueprintPure, Category = UI)
	virtual bool CanHandleUICommand() const { return ISoUIEventHandler::Execute_IsOpened(this); }

	UFUNCTION(BlueprintPure, Category = UI)
	USoUIButtonArray* GetButtonArray() const;

	UFUNCTION(BlueprintPure, Category = UI)
	USoUIButtonImageArray* GetButtonArrayAsButtonImagesArray() const;

	UFUNCTION(BlueprintCallable, Category = UI)
	void UpdateEnsureOnlyTheseCommandsExist(const TArray<FSoUITextCommandPair> CommandsToBeDisplayed);

	UFUNCTION(BlueprintCallable, Category = UI)
	virtual void RefreshButtonsArray() {}

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = ">Events", meta = (DisplayName = "OnPressedButtonTooltipsCommand"))
	bool ReceiveOnPressedButtonTooltipsCommand(ESoUICommand Command);

	UFUNCTION(BlueprintCallable, Category = ">Events")
	virtual bool OnPressedButtonTooltipsCommand(ESoUICommand Command)
	{
		return ReceiveOnPressedButtonTooltipsCommand(Command);
	}

	FSoUIOpenChangedEvent& OnOpenChangedEvent() { return OpenChangedEvent; }
	FSoConfirmPanelQuestionAnsweredEvent& OnQuestionAnsweredEvent() { return QuestionAnsweredEvent; }

protected:
	UPROPERTY(BlueprintAssignable, Category = ">Events")
	FSoUIOpenChangedEvent OpenChangedEvent;

	UPROPERTY(BlueprintAssignable, Category = ">Events")
	FSoConfirmPanelQuestionAnsweredEvent QuestionAnsweredEvent;

	UPROPERTY(BlueprintReadOnly, Category = ">State")
	bool bOpened = false;

	// Buttons array that can be used for example for Yes/No Buttons
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = ">Widgets")
	USoUIButtonArray* ButtonsArray = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">Behaviour")
	bool bBehaveAsCommandTooltips = false;
};

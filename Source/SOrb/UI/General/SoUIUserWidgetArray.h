// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Blueprint/UserWidget.h"
#include "UI/General/SoUITypes.h"
#include "Delegates/IDelegateInstance.h"
#include "SoUIUserWidgetBaseNavigation.h"

#include "SoUIUserWidgetArray.generated.h"

class USoUIButton;
class UFMODEvent;
class USoUIUserWidget;
class ASoCharacter;
class USoUIUserWidgetArray;


/**
 * Container class containing an array of UserWidgets
 *
 * the layout can be ordered horizontally or vertically
 * it supports selecting/highlighting the widgets, navigation for the active button, etc.
 */
UCLASS()
class SORB_API USoUIUserWidgetArray : public USoUIUserWidgetBaseNavigation
{
	GENERATED_BODY()

public:
	USoUIUserWidgetArray(const FObjectInitializer& ObjectInitializer);

	//
	// UObject Interface
	//
	void PostLoad() override;

	//
	// UUserWidget Interface
	//
	void SynchronizeProperties() override;
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	void NativeConstruct() override;
	void NativeDestruct() override;

	//
	// USoUIUserWidgetBaseNavigation interface
	//

	/**
	* Navigate along the children
	* @Param Command: defines the navigation direction depending on the layout orientation (vertical/horizontal)
	* Returns true if the command was handled
	*/
	bool Navigate(ESoUICommand Command, bool bPlaySound = true) override;

	// Pressed the current widget, either mouse or enter
	bool NavigateOnPressed(bool bPlaySound = true) override;

	//
	// Own methods
	//

	// Useful to know after calling Navigate() if the child changed because of it
	bool WasChildChangedInNavigate() const { return bWasChildChangedInNavigate; }

	/** Sets the currently selected widget and highlights it */
	UFUNCTION(BlueprintCallable, Category = ">Selection")
	virtual void SetSelectedIndex(int32 ChildIndex, bool bPlaySound = false);

	/** returns with the currently selected widget, index might be INDEX_NONE */
	UFUNCTION(BlueprintPure, Category = ">Selection")
	int32 GetSelectedIndex() const { return SelectedIndex; }
	int32 GetPreviousSelectedIndex() const { return PreviousSelectedIndex; }

	/** Puts the active index to OutIndex, return value tells if it is a valid value or not */
	UFUNCTION(BlueprintCallable, Category = ">Selection")
	bool GetSelectedIndexChecked(int32& OutIndex) const;

	// Resets the selected child (unhighlight it) if it is deactivated.
	// Returns true if something new and valid was selected
	// Returns false if the current SelectedIndex is valid
	// NOTE: most likely you should just call Highlight() this widget
	virtual bool SelectFirstValidChild(bool bPlaySound = false);

	/** @Return: the index of the new selected widget, or INDEX_NONE if there wasn't any active */
	virtual int32 SelectFirstActiveChild(int32 StartFrom = 0);

	/**
	 *  Enables/Disables the highlight for the selected child
	 *  The value is not cached, so if it should be disabled
	 *  the function has to be called manually after each SetSelectedElement() call
	 */
	UFUNCTION(BlueprintCallable, Category = ">Selection")
	void HighlightSelectedChild(bool bPlaySound = false) { SetSelectedChildIsHighlighted(true, bPlaySound);  }

	UFUNCTION(BlueprintCallable, Category = ">Selection")
	void UnhighlightSelectedChild(bool bPlaySound = false) { SetSelectedChildIsHighlighted(false, bPlaySound); }

	// Change child states - has nothing to do with the child array itself:

	UFUNCTION(BlueprintCallable, Category = ">Selection")
	void ActivateChildAt(int32 ChildIndex, bool bPlaySound = false) { SetIsActiveOnChildAt(ChildIndex, true, bPlaySound); }

	UFUNCTION(BlueprintCallable, Category = ">Selection")
	void DeactivateChildAt(int32 ChildIndex, bool bPlaySound = false) { SetIsActiveOnChildAt(ChildIndex, false, bPlaySound); }

	UFUNCTION(BlueprintCallable, Category = ">Selection")
	bool HighlightChildAt(int32 ChildIndex, bool bPlaySound = false) { return SetIsHighlightedOnChildAt(ChildIndex, true, bPlaySound); }

	UFUNCTION(BlueprintCallable, Category = ">Selection")
	bool UnhighlightChildAt(int32 ChildIndex, bool bPlaySound = false) { return SetIsHighlightedOnChildAt(ChildIndex, false, bPlaySound); }

	UFUNCTION(BlueprintCallable, Category = ">Selection")
	void ActivateAllChildren() { SetIsActiveOnAllChildren(true); }

	UFUNCTION(BlueprintCallable, Category = ">Selection")
	void DeactivateAllChildren() { SetIsActiveOnAllChildren(false); }

	void HighlightAllChildren() { SetIsHighlightedOnAllChildren(true); }
	void UnhighlightAllChildren() { SetIsHighlightedOnAllChildren(false); }


	//
	// Operations for ContainerChildren
	//

	UFUNCTION(BlueprintPure, Category = ">Children")
	virtual int32 NumContainerChildren() const { return ContainerChildren.Num(); }

	virtual void AddContainerChild(UUserWidget* Child);
	virtual void RemoveContainerChild(UUserWidget* Child);
	virtual void RemoveContainerChildAt(int32 ChildIndex);
	virtual void EmptyContainerChildren(bool bContainer = false);
	virtual UUserWidget* GetContainerChildAt(int32 ChildIndex) const;
	virtual bool IsIndexForContainer(int32 ChildIndex) const { return ContainerChildren.IsValidIndex(ChildIndex); }

	// NOTE use with care because the IsValidWidget() will tell us this widget is it true
	// But the container is hidden
	virtual void HideContainer();
	virtual void ShowContainer();

	//
	// Operations for ExtraAttachedChildren
	//
	void AddExtraAttachedChild(UUserWidget* ExtraChild);
	void EmptyExtraAttachedChildren();
	const TArray<UUserWidget*>& GetExtraAttachedChildren() const { return ExtraAttachedChildren; }
	UUserWidget* GetExtraAttachedChildAt(int32 ChildIndex) const;
	int32 NumExtraAttachedChildren() const { return ExtraAttachedChildren.Num(); }

	// Same as SetSelectedIndex only this works exclusively on the ExtraAttached children
	UFUNCTION(BlueprintCallable, Category = ">Selection")
	void SetSelectedExtraAttachedIndex(int32 ExtraAttachedChildIndex, bool bPlaySound = false)
	{
		SetSelectedIndex(ConvertExtraAttachedChildIndexToAllChildIndex(ExtraAttachedChildIndex), bPlaySound);
	}

	// The parameter ChildIndex is a valid index in all the children [0, NumAllChildren)
	// Returns the index which is valid in the ExtraAttachedChildren Array [0, NumExtraAttachedChildren)
	int32 ConvertAllChildIndexToExtraAttachedChildIndex(int32 ChildIndex) const { return ChildIndex - NumContainerChildren(); }

	// The parameter ExtraAttachedChildIndex is a valid index in the ExtraAttachedChildren Array [0, NumExtraAttachedChildren)
	// Returns the index which is valid in all the children  [0, NumAllChildren)
	int32 ConvertExtraAttachedChildIndexToAllChildIndex(int32 ExtraAttachedChildIndex) const { return NumContainerChildren() + ExtraAttachedChildIndex; }

	bool IsIndexForExtraAttachedChildren(int32 ChildIndex) const
	{
		return ExtraAttachedChildren.IsValidIndex(ConvertAllChildIndexToExtraAttachedChildIndex(ChildIndex));
	}

	// Is the index at the margins of the ExtraAttachedChildren array [0, Num)
	bool IsMarginIndexForExtraAttachedChildren(int32 ChildIndex) const
	{
		const int32 NormalizedIndex = ConvertAllChildIndexToExtraAttachedChildIndex(ChildIndex);
		return ExtraAttachedChildren.IsValidIndex(NormalizedIndex) && (NormalizedIndex == 0 || NormalizedIndex == ExtraAttachedChildren.Num() - 1);
	}

	//
	// Operations for ContainerChildren + ExtraAttachedChildren
	//

	static bool IsHorizontalCommand(ESoUICommand Command)
	{
		return Command == ESoUICommand::EUC_Left || Command == ESoUICommand::EUC_Right;
	}
	static bool IsVerticalCommand(ESoUICommand Command)
	{
		return Command == ESoUICommand::EUC_Up || Command == ESoUICommand::EUC_Down;
	}

	// Tells us if the layouts are in sync
	bool HasTwoDifferentLayouts() const
	{
		if (NumExtraAttachedChildren() > 0)
			return bVerticalLayout != bExtraAttachedVerticalLayout;

		// We only use the ContainerChildren so this is a hard no
		return false;
	}

	// Is the ButtonIndex a valid index for the buttons
	// NOTE: This counts all children, so ContainerChildren + ExtraAttachedChildren
	UFUNCTION(BlueprintPure, Category = ">Children")
	bool IsValidIndex(int32 ChildIndex) const;

	bool IsValidWidgetAt(int32 ChildIndex) const { return IsWidgetValid(GetChildAt(ChildIndex)); }

	// Gets the number of children
	// NOTE: This counts all children, so ContainerChildren + ExtraAttachedChildren
	UFUNCTION(BlueprintPure, Category = ">Children")
	int32 NumAllChildren() const { return NumContainerChildren() + NumExtraAttachedChildren(); }

	// NOTE: This counts all children, so ContainerChildren + ExtraAttachedChildren
	UFUNCTION(BlueprintPure, Category = ">Children")
	UUserWidget* GetChildAt(int32 ChildIndex) const;
	USoUIUserWidget* GetChildAtAsSoUserWidget(int32 ChildIndex) const;
	USoUIUserWidgetArray* GetChildAtAsSoUserWidgetArray(int32 ChildIndex) const;

	UFUNCTION(BlueprintPure, Category = ">Children")
	bool IsEmpty() const { return NumAllChildren() == 0; }

	//
	// NOTE: Not BP on purpose
	//

	// Active
	virtual bool IsChildActive(int32 ChildIndex) const { return IsWidgetActive(GetChildAt(ChildIndex)); }
	virtual void SetIsActiveOnAllChildren(bool bActive)
	{
		const int32 NumChildren = NumAllChildren();
		for (int32 Index = 0; Index < NumChildren; ++Index)
			SetIsActiveOnChildAt(Index, bActive);
	}
	virtual bool SetIsActiveOnChildAt(int32 ChildIndex, bool bActive, bool bPlaySound = false)
	{
		return SetIsActiveOnWidget(GetChildAt(ChildIndex), bActive, bPlaySound);
	}

	bool IsSelectedChildActive() const { return IsChildActive(GetSelectedIndex()); }
	bool SetSelectedChildIsActive(bool bActive, bool bPlaySound = false) { return SetIsActiveOnChildAt(GetSelectedIndex(), bActive, bPlaySound); }

	// Highlight
	bool SetIsHighlighted(bool bInHighlighted, bool bPlaySound = false) override;
	virtual bool IsChildHighlighted(int32 ChildIndex) const { return IsWidgetHighlighted(GetChildAt(ChildIndex)); }
	virtual void SetIsHighlightedOnAllChildren(bool bInHighlighted)
	{
		UE_LOG(
			LogSoUI,
			Verbose,
			TEXT("[%s] USoUIUserWidgetArray::SetIsHighlightedOnAllChildren(bInHighlighted = %d)"),
			*GetName(), bInHighlighted
		);

		const int32 NumChildren = NumAllChildren();
		for (int32 Index = 0; Index < NumChildren; ++Index)
			SetIsHighlightedOnChildAt(Index, bInHighlighted);
	}
	virtual bool SetIsHighlightedOnChildAt(int32 ChildIndex, bool bHighlight, bool bPlaySound = false);
	virtual bool IsAnyChildHighlighted() const
	{
		const int32 NumChildren = NumAllChildren();
		for (int32 Index = 0; Index < NumChildren; ++Index)
			if (IsChildHighlighted(Index))
				return true;

		return false;
	}

	bool IsSelectedChildHighlighted() const { return IsChildHighlighted(GetSelectedIndex()); }
	bool SetSelectedChildIsHighlighted(bool bHighlight, bool bPlaySound = false) { return SetIsHighlightedOnChildAt(GetSelectedIndex(), bHighlight, bPlaySound); }

	// Enabled
	virtual bool IsChildEnabled(int32 ChildIndex) const { return IsWidgetEnabled(GetChildAt(ChildIndex)); }
	virtual bool SetIsEnabledOnChildAt(int32 ChildIndex, bool bEnabled) { return SetIsEnabledOnWidget(GetChildAt(ChildIndex), bEnabled); }

	bool IsSelectedChildEnabled() const { return IsChildEnabled(GetSelectedIndex()); }
	bool SetSelectedChildIsEnabled(bool bEnabled) { return SetIsEnabledOnChildAt(GetSelectedIndex(), bEnabled); }

	// Navigation
	virtual bool NavigateOnChildAt(int32 ChildIndex, ESoUICommand Command, bool bPlaySound = true)
	{
		return NavigateOnWidget(GetChildAt(ChildIndex), Command, bPlaySound);
	}

	//
	// Getters
	//
	bool IsVerticalLayout() const { return bVerticalLayout; }

	//
	// Setters
	//

	ThisClass* SetCyclicNavigation(bool bEnable)
	{
		bCyclicNavigation = bEnable;
		return this;
	}
	ThisClass* SetVerticalLayout(bool bEnable)
	{
		bVerticalLayout = bEnable;
		return this;
	}
	ThisClass* SetExtraAttachedVerticalLayout(bool bEnable)
	{
		bExtraAttachedVerticalLayout = bEnable;
		return this;
	}
	ThisClass* SetSkipDeactivatedChildren(bool bEnable)
	{
		bSkipDeactivatedChildren = bEnable;
		return this;
	}
	ThisClass* SetUseCustomPadding(bool bEnable, const FMargin& InPadding)
	{
		bUseCustomPadding = bEnable;
		PaddingForEachChildren = InPadding;
		return this;
	}
	ThisClass* SetTrackPressedInput(bool bEnable)
	{
		bTrackPressedInput = bEnable;
		return this;
	}
	ThisClass* SetFakeVirtualContainer(bool bEnable)
	{
		bFakeVirtualContainer = bEnable;
		return this;
	}

	ThisClass* SetSFXChildSelectionChanged(UFMODEvent* SFX)
	{
		SFXChildSelectionChanged = SFX;
		return this;
	}
	ThisClass* SetSFXChildPressed(UFMODEvent* SFX)
	{
		SFXChildPressed = SFX;
		return this;
	}

	// Events
	FSoNavigateOnPressedHandleChilEventCPP& OnNavigateOnPressedHandleChildEvent() { return NavigateOnPressedHandleChildEventCPP; }
	FSoNavigateOnPressedHandleChildArrayEventCPP& OnNavigateOnPressedHandleChildArrayEvent() { return NavigateOnPressedHandleChildArrayEventCPP; }
	FSoPostSetSelectedIndexEvent& OnPostSetSelectedIndexEvent() { return PostSetSelectedIndexEvent; }
	FSoPostSetSelectedIndexEvent& OnSelectedChildChangedEvent() { return PostSetSelectedIndexEvent; }

	FSoArrayChildBoolChangedEventCPP& OnChildHighlightChangedEvent() { return ProxyChildHighlightChangedEventCPP; }
	FSoArrayChildBoolChangedEventCPP& OnChildEnabledChangedEvent() { return ProxyChildEnabledChangedEventCPP; }

protected:
	static bool IsWidgetEnabled(UUserWidget* Widget);
	static bool IsWidgetValid(UUserWidget* Widget);
	static bool IsWidgetActive(UUserWidget* Widget);
	static bool IsWidgetHighlighted(UUserWidget* Widget);
	static bool SetIsEnabledOnWidget(UUserWidget* Widget, bool bEnabled);
	static bool SetIsHighlightedOnWidget(UUserWidget* Widget, bool bInHighlighted, bool bPlaySound = false);
	static bool SetIsActiveOnWidget(UUserWidget* Widget, bool bActive, bool bPlaySound = false);
	static bool NavigateOnWidget(UUserWidget* Widget, ESoUICommand Command, bool bPlaySound = true);

	//
	// USoUIUserWidgetBaseNavigation Interface
	//

	FReply OnUIEventType(ESoUIEventType EventType) override;

	//
	// Own methods
	//

	bool InternalCanUseContainer() const { return !bFakeVirtualContainer && Container; }
	void InternalSetSelectedIndex(int32 NewIndex);

	// Normal navigation on only children
	//
	// Or if we have children with layout bVertical and and extra attached child that is an Array is with layout !bVertical
	// This would also work because the direction of the extra attached child is opposite so the directions are mutually exclusive.
	// Aka this would return 0, which means nothing is modified and we would navigate ON the extra attached child Array
	// Example of this is in the Settings.
	static int32 InternalGetDirectChildrenDeltaFromCommand(ESoUICommand Command, bool bIsOnVerticalLayout);

	bool InternalGetDeltaFromCommand(ESoUICommand Command, int32 OldIndex, int32& OutDelta) const;
	int32 InternalGetNextChildIndex(int32 Delta, int32 CurrentIndex);

	void EnsureHighlights();

	// Listen/unlisten to event from the children
	void SubscribeToChildEvents(UUserWidget* Child, int32 ChildIndex);
	void UnSubscribeFromChildEvents(UUserWidget* Child);

	// Child requested we change the highlight. Most likely mouse or touch event
	// Children -> Parent (this)
	UFUNCTION()
	virtual void OnRequestChildHighlightChange(int32 ChildIndex, bool bHighlight);

	UFUNCTION()
	virtual void OnRequestChildPressed(int32 ChildIndex);

	virtual void UpdateAllOverrides();
	virtual void UpdateWidgetOverrides(UUserWidget* Widget, int32 ChildIndex);
	void UpdateOverridesSFX(UUserWidget* Widget);
	void UpdateOverrideCustomPadding(UUserWidget* Widget);
	void MapOverContainerChildren(TFunction<void(int32, UUserWidget*)> CallbackLoopBody);

	//
	// Handle CPP proxies
	//
	UFUNCTION()
	void OnProxyChildHighlightChangedEvent(int32 ChildIndex, bool bNewValue) { ProxyChildHighlightChangedEventCPP.Broadcast(ChildIndex, bNewValue); }
	UFUNCTION()
	void OnProxyChildEnabledChangedEvent(int32 ChildIndex, bool bNewValue) { ProxyChildEnabledChangedEventCPP.Broadcast(ChildIndex, bNewValue); }

public:
	//
	// Handle BP events and events firing
	//

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = ">Events", DisplayName = "OnChildHighlightChanged")
	void ReceiveOnChildHighlightChanged(int32 ChildIndex, bool bHighlight);
	UFUNCTION(BlueprintCallable, Category = ">Events")
	virtual void OnChildHighlightChanged(int32 ChildIndex, bool bHighlight);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = ">Events", DisplayName = "OnChildEnabledChanged")
	void ReceiveOnChildEnabledChanged(int32 ChildIndex, bool bEnabled);
	UFUNCTION(BlueprintCallable, Category = ">Events")
	virtual void OnChildEnabledChanged(int32 ChildIndex, bool bEnabled);

protected:
	//
	// Events
	//

	// Handle NavigateOnPressed() on a simple widget
	UPROPERTY(BlueprintAssignable, Category = ">Events", DisplayName = "OnNavigateOnPressedHandleChildEvent")
	FSoNavigateOnPressedHandleChilEvent NavigateOnPressedHandleChildEvent;
	// CPP variant
	FSoNavigateOnPressedHandleChilEventCPP NavigateOnPressedHandleChildEventCPP;

	// Handle NavigateOnPressed() on an Array Widget
	UPROPERTY(BlueprintAssignable, Category = ">Events", DisplayName = "OnNavigateOnPressedHandleChildArrayEvent")
	FSoNavigateOnPressedHandleChildArrayEvent NavigateOnPressedHandleChildArrayEvent;
	// CPP variant
	FSoNavigateOnPressedHandleChildArrayEventCPP NavigateOnPressedHandleChildArrayEventCPP;
	// Broadcasts after setting the SelectedIndex
	FSoPostSetSelectedIndexEvent PostSetSelectedIndexEvent;

	// Child Highlight changed
	UPROPERTY(BlueprintAssignable, Category = ">Events", DisplayName = "OnChildHighlightChangedEvent")
	FSoArrayChildBoolChangedEvent ChildHighlightChangedEvent;

	// Child Enabled changed
	UPROPERTY(BlueprintAssignable, Category = ">Events", DisplayName = "OnChildEnabledChangedEvent")
	FSoArrayChildBoolChangedEvent ChildEnabledChangedEvent;

	// Proxy events that pass from Dynamic event to our event handler in CPP
	FSoArrayChildBoolChangedEventCPP ProxyChildHighlightChangedEventCPP;
	FSoArrayChildBoolChangedEventCPP ProxyChildEnabledChangedEventCPP;


	//
	// Variables
	//

	// Could be Vertical/Horizontal,
	// NOTE: can contain Non-Button elements
	UPROPERTY(BlueprintReadOnly, Category = ">Container", meta = (BindWidget))
	UPanelWidget* Container = nullptr;

	/**
	 * Actual buttons - maybe not all of them are displayed. But these are in the Container
	 * NOTE: most of the time all of this correspond to the Container children
	 * But in some cases they can differ
	 */
	UPROPERTY(BlueprintReadOnly, Category = ">Children")
	TArray<UUserWidget*> ContainerChildren;

	/**
	 * Extra Children that are attached to this class but are NOT in the Container.
	 * This allows us to have elements that are separate from the Container
	 */
	UPROPERTY(BlueprintReadOnly, Category = ">Children")
	TArray<UUserWidget*> ExtraAttachedChildren;

	UPROPERTY(BlueprintReadOnly, Category = ">Runtime")
	bool bWasChildChangedInNavigate = false;

	//
	// Container
	//

	// If true we won't use the Container of this widget
	// It means the widgets are in some other container maybe external to this widget
	// NOTE: This essentially makes the ContainerChildren the same as the ExtraAttachedChildren
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Container")
	bool bFakeVirtualContainer = false;

	//
	// Selection
	//

	// If true this means if we unhighlight all children we will highlight the selected child (if any)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Selection")
	bool bAlwaysHighlightAtLeastOneChild = false;

	// In most cases you want the highlight to also select the child index
	// If true: OnRequestChildHighlightChange will highlight and select
	// If false:
	// - highlight - OnRequestChildHighlightChange
	// - select - OnRequestChildPressed or Navigation
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Selection")
	bool bHighlightSelectsChild = true;

	//
	// Children
	//

	// Only used when creating the widgets ourselves in this class
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Children")
	TSubclassOf<UUserWidget> ChildWidgetClass;

	// Depending on the value after the last element the selected is either the first or none
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Children")
	bool bCyclicNavigation = false;

	// Decides if the navigation is controlled by the left/right or the up/down commands, only affects ContainerChildren
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Children")
	bool bVerticalLayout = false;

	// Same as bVerticalLayout but only affects ExtraAttachedChildren
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Children")
	bool bExtraAttachedVerticalLayout = false;

	// Decides if we should skip the children highlight for buttons that are not active.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Children")
	bool bSkipDeactivatedChildren = false;

	//
	// Override
	//

	// Should we use PaddingForEachChildren
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Override Padding")
	bool bUseCustomPadding = false;

	// Padding applied to each children
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Override Padding")
	FMargin PaddingForEachChildren;


	//
	// Input
	//

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">Input")
	FSoUITrackedInputKeyDown TrackedInput;

	// Enable moving navigation if a certain button is pressed
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">Input")
	bool bTrackPressedInput = false;

	// Did we wrap around in the last Navigate
	UPROPERTY(BlueprintReadOnly, Category = ">Input")
	bool bWasLastChildWrappedAround = false;

	// Runtime values
	// Currently selected button index inside the Children Array
	int32 SelectedIndex = INDEX_NONE;

	// The value of previous SelectedIndex
	int32 PreviousSelectedIndex = INDEX_NONE;

	//
	// SFX
	//

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">SFX")
	bool bSFXSetDefault = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXChildSelectionChanged = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXChildPressed = nullptr;

	// Override the individual buttons sounds with our sounds
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">SFX")
	bool bSFXOverrideIndividualChildren = true;

	//
	// Cached
	//

	UPROPERTY()
	ASoCharacter* SoCharacter = nullptr;

	//
	// Keep track of delegates
	//
	TMap<uint32, FDelegateHandle> DelegatesEnabledChangedMap;
	TMap<uint32, FDelegateHandle> DelegatesHighlightChangedMap;
	TMap<uint32, FDelegateHandle> DelegatesHighlightRequestChangeMap;
	TMap<uint32, FDelegateHandle> DelegatesPressedRequestMap;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Events")
	bool bListenToChildHighlightRequest = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Events")
	bool bListenToChildPressedRequest = true;
};

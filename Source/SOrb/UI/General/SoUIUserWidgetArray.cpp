// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoUIUserWidgetArray.h"

#include "UI/General/SoUITypes.h"
#include "Basic/Helpers/SoMathHelper.h"
#include "UI/SoUIHelper.h"
#include "SoUIUserWidget.h"
#include "Basic/SoAudioManager.h"
#include "Components/HorizontalBox.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Character/SoCharacter.h"
#include "Components/VerticalBox.h"
#include "Basic/SoGameSingleton.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUIUserWidgetArray::USoUIUserWidgetArray(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	TrackedInput.KeyDownThresholdSeconds = 0.2f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetArray::PostLoad()
{
	Super::PostLoad();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetArray::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	UpdateAllOverrides();

	// Vertical box, vertical layout, makes sense
	if (InternalCanUseContainer() && Container->IsA<UVerticalBox>())
	{
		bVerticalLayout = true;
	}

	// SFX
	if (bSFXSetDefault)
	{
		if (SFXChildSelectionChanged == nullptr)
			SFXChildSelectionChanged = USoGameSingleton::GetSFX(ESoSFX::MenuButtonSwitch);

		if (SFXChildPressed == nullptr)
			SFXChildPressed = USoGameSingleton::GetSFX(ESoSFX::MenuButtonPressed);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetArray::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!bTrackPressedInput)
		return;

	if (!SoCharacter || !IsValidWidget() || !SoCharacter->IsAnyUIInputPressed())
	{
		TrackedInput.Reset();
		return;
	}

	// Pressed left/right/up/down change value
	if (bVerticalLayout)
	{
		TrackedInput.Tick(InDeltaTime, [&]()
		{
			if (SoCharacter->AreAnyUIInputsPressed({ ESoUICommand::EUC_Up, ESoUICommand::EUC_MainMenuUp }))
				Navigate(ESoUICommand::EUC_Up);
			else if (SoCharacter->AreAnyUIInputsPressed({ ESoUICommand::EUC_Down, ESoUICommand::EUC_MainMenuDown }))
				Navigate(ESoUICommand::EUC_Down);
		});
	}
	else
	{
		TrackedInput.Tick(InDeltaTime, [&]()
		{
			if (SoCharacter->AreAnyUIInputsPressed({ ESoUICommand::EUC_Left, ESoUICommand::EUC_MainMenuLeft }))
				Navigate(ESoUICommand::EUC_Left);
			else if (SoCharacter->AreAnyUIInputsPressed({ ESoUICommand::EUC_Right, ESoUICommand::EUC_MainMenuRight }))
				Navigate(ESoUICommand::EUC_Right);
		});
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetArray::NativeConstruct()
{
	// Call our CPP
	ChildHighlightChangedEvent.AddDynamic(this, &ThisClass::OnProxyChildHighlightChangedEvent);
	ChildEnabledChangedEvent.AddDynamic(this, &ThisClass::OnProxyChildEnabledChangedEvent);

	Super::NativeConstruct();

	SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);

	// Default to 0
	SetSelectedIndex(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetArray::NativeDestruct()
{
	ChildHighlightChangedEvent.RemoveDynamic(this, &ThisClass::OnProxyChildHighlightChangedEvent);
	ChildEnabledChangedEvent.RemoveDynamic(this, &ThisClass::OnProxyChildEnabledChangedEvent);

	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIUserWidgetArray::Navigate(ESoUICommand Command, bool bPlaySound)
{
	// Ignore navigation, widget is disabled
	bWasChildChangedInNavigate = false;
	if (!GetIsEnabled())
		return false;

	PreNavigateEvent.Broadcast();

	// Handle OnPressed
	bool bStatus = false;
	if (Command == ESoUICommand::EUC_MainMenuEnter)
	{
		bStatus = NavigateOnPressed(bPlaySound);
		PostNavigateEvent.Broadcast(bStatus);
		return bStatus;
	}

	// Can we navigate on a different child
	// OR we should navigate on THAT child
	int32 Delta = 0;
	const int32 OldIndex = GetSelectedIndex();
	const bool bCanNavigateChildren = InternalGetDeltaFromCommand(Command, OldIndex, Delta);
	if (bCanNavigateChildren)
	{
		// Try to navigate on children
		const int32 NewIndex = InternalGetNextChildIndex(Delta, OldIndex);

		// Not found
		if (!IsValidIndex(NewIndex))
		{
			PostNavigateEvent.Broadcast(false);
			return false;
		}

		// Found
		SetSelectedIndex(NewIndex, bPlaySound);
		bStatus = OldIndex != NewIndex;
		bWasChildChangedInNavigate = bStatus;

		// TOOD maybe only play this in Highlight?
		//if (bPlaySound && bStatus)
		//	USoAudioManager::PlaySound2D(this, SFXChildSelectionChanged);
	}
	else
	{
		// Navigate ON our current child
		bStatus = NavigateOnChildAt(OldIndex, Command, bPlaySound);
	}

	// Special case for keyboard
	if (bAlwaysHighlightAtLeastOneChild && bWasChildChangedInNavigate)
	{
		UnhighlightAllChildren();
	}

	PostNavigateEvent.Broadcast(bStatus);
	return bStatus;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIUserWidgetArray::NavigateOnPressed(bool bPlaySound)
{
	UUserWidget* Widget = GetChildAt(GetSelectedIndex());

	// Handle arrays differently
	if (ThisClass* SoArray = Cast<ThisClass>(Widget))
	{
		if (NavigateOnPressedHandleChildArrayEvent.IsBound())
		{
			// Handled by BP
			NavigateOnPressedHandleChildArrayEvent.Broadcast(GetSelectedIndex(), SoArray);
			return true;
		}
		if (NavigateOnPressedHandleChildArrayEventCPP.IsBound())
		{
			// Handled by CPP
			NavigateOnPressedHandleChildArrayEventCPP.Execute(GetSelectedIndex(), SoArray);
			return true;
		}

		// Default handle it
		return SoArray->NavigateOnPressed(bPlaySound);
	}

	if (USoUIUserWidget* SoUserWidget = Cast<USoUIUserWidget>(Widget))
	{
		if (NavigateOnPressedHandleChildEvent.IsBound())
		{
			// Handled by BP
			NavigateOnPressedHandleChildEvent.Broadcast(GetSelectedIndex(), SoUserWidget);
		}
		else if (NavigateOnPressedHandleChildEventCPP.IsBound())
		{
			// Handled by CPP
			NavigateOnPressedHandleChildEventCPP.Execute(GetSelectedIndex(), SoUserWidget);
		}
		else
		{
			// Default handle it
			SoUserWidget->NavigateOnPressed(bPlaySound);
		}
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoUIUserWidgetArray::InternalGetDirectChildrenDeltaFromCommand(ESoUICommand Command, bool bIsOnVerticalLayout)
{
	if (bIsOnVerticalLayout)
	{
		// Vertical
		if (Command == ESoUICommand::EUC_Up)
			return -1;
		if (Command == ESoUICommand::EUC_Down)
			return 1;
	}
	else
	{
		// Horizontal
		if (Command == ESoUICommand::EUC_Left)
			return -1;
		if (Command == ESoUICommand::EUC_Right)
			return 1;
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIUserWidgetArray::InternalGetDeltaFromCommand(ESoUICommand Command, int32 OldIndex, int32& OutDelta) const
{
	// NOTE: we mostly assume that none of the extra attached children are Arrays
	OutDelta = 0;

	// Choose direction based on layout vertical or horizontal
	Command = USoUIHelper::TryTranslateMenuCommandDirectionToGame(Command);
	bool bIsOnVerticalLayout = bVerticalLayout;
	bool bSwitchedLayout = false;

	// The layout of our extra attached children is opposite
	// We must handle a custom layout
	const bool bIsOnExtraChildren = IsIndexForExtraAttachedChildren(OldIndex);
	if (bIsOnExtraChildren)
	{
		if (HasTwoDifferentLayouts())
		{
			// Extra attached children has a different layout
			bIsOnVerticalLayout = !bIsOnVerticalLayout;
			bSwitchedLayout = true;
		}
		else
		{
			// Same layout

			// Special case when we only have one extra attached child that is also an array
			USoUIUserWidgetArray* ExtraChildArray = Cast<ThisClass> (GetExtraAttachedChildAt(OldIndex));
			if (ExtraChildArray && NumExtraAttachedChildren() == 1)
			{
				const int32 ChildDelta = InternalGetDirectChildrenDeltaFromCommand(Command, bIsOnVerticalLayout);

				// The delta is possible, let the child handle it
				if (ExtraChildArray->IsValidIndex(ExtraChildArray->GetSelectedIndex() + ChildDelta))
					return false;
			}
		}
	}

	// Normal
	OutDelta = InternalGetDirectChildrenDeltaFromCommand(Command, bIsOnVerticalLayout);

	// Special layout switched case
	if (bSwitchedLayout)
	{
		// Extra attached children are in opposite direction and we are on them
		if (bIsOnVerticalLayout)
		{
			// NOTE: Assumes OldIndex is in the extra children
			checkNoEntry();
		}
		else
		{
			// Horizontal
			// Children are Vertical
			// Extra Attached Children Are Horizontal

			// NOTE: Assumes OldIndex is in the extra children
			// TODO: it assumes the extra children are below the container children
			if (IsVerticalCommand(Command))
			{
				// Should have navigated the container children
				// Go up or down to either first or last element in the ContainerChildren
				if (Command == ESoUICommand::EUC_Up)
				{
					// To last
					OutDelta = -(OldIndex - (NumContainerChildren() - 1));
				}
				else if (Command == ESoUICommand::EUC_Down)
				{
					// To first
					// (NumAllChildren() - 1) - OldIndex + 1 =
					OutDelta = NumAllChildren() - OldIndex;
				}
			}
			else
			{
				// Horizontal Command
				// Should have navigated the extra children
				const int32 NewIndex = OldIndex + OutDelta;
				if (!IsIndexForExtraAttachedChildren(NewIndex))
				{
					// Make sure we wrap around
					if (Command == ESoUICommand::EUC_Left)
					{
						// To Last
						OutDelta = NumAllChildren() - OldIndex - 1;
					}
					else if (Command == ESoUICommand::EUC_Right)
					{
						// To First
						OutDelta = -(OldIndex - NumContainerChildren());
					}
				}
				// Else use the normal child direction if it is on extra attached child
			}
		}
	}

	// If still 0 it means it was not a direction for our layout
	return OutDelta != 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoUIUserWidgetArray::InternalGetNextChildIndex(int32 Delta, int32 CurrentIndex)
{
	// Not a direction command
	if (Delta == 0)
		return INDEX_NONE;

	bWasLastChildWrappedAround = false;
	const int32 NumChildren = NumAllChildren();

	// Advance the selected index
	int32 NextIndex = CurrentIndex;
	auto ChooseNextIndex = [this, &NumChildren, &NextIndex, &Delta]()
	{
		NextIndex += Delta;
		if (bCyclicNavigation && NumChildren != 0)
			NextIndex = USoMathHelper::WrapIndexAround(NextIndex, NumChildren);
		else
			NextIndex = FMath::Clamp(NextIndex, 0, NumChildren - 1);
	};

	// Try first
	ChooseNextIndex();

	// Try to skip deactivated buttons
	if (bSkipDeactivatedChildren)
	{
		// Try to skip deactivated buttons
		int32 NumTries = -1;
		while (true)
		{
			// Reached limit of tries
			if (NumTries >= NumChildren)
				break;

			// Got a valid index
			if (IsValidWidgetAt(NextIndex))
				break;

			NumTries++;
			ChooseNextIndex();
		}
	}

	// Check wrapping around
	if (NextIndex != CurrentIndex)
	{
		const bool bLastToFirst = CurrentIndex == NumChildren - 1 && NextIndex == 0;
		const bool bFirstToLast = CurrentIndex == 0               && NextIndex == NumChildren - 1;
		bWasLastChildWrappedAround = bLastToFirst || bFirstToLast;
	}

	return NextIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetArray::EnsureHighlights()
{
	if (!bIsConstructed)
		return;

	// Don't care
	if (!bAlwaysHighlightAtLeastOneChild)
		return;

	// One child is already highlighted
	if (IsAnyChildHighlighted())
		return;

	SetSelectedChildIsHighlighted(true, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FReply USoUIUserWidgetArray::OnUIEventType(ESoUIEventType EventType)
{
	return FReply::Unhandled();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetArray::InternalSetSelectedIndex(int32 NewIndex)
{
	PreviousSelectedIndex = SelectedIndex;
	SelectedIndex = NewIndex;
	PostSetSelectedIndexEvent.Broadcast(PreviousSelectedIndex, NewIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetArray::SetSelectedIndex(int32 ChildIndex, bool bPlaySound)
{
	// Can't select as it is not active
	if (bSkipDeactivatedChildren && !IsValidWidgetAt(ChildIndex))
		return;

	UE_LOG(
		LogSoUI,
		Verbose,
		TEXT("[%s] USoUIUserWidgetArray::SetSelectedIndex(ChildIndex = %d, bPlaySound = %d)"),
		*GetName(), ChildIndex, bPlaySound
	);

	// We must disable bAlwaysHighlightAtLeastOneChild for this to work properly
	const bool bPreviousValue = bAlwaysHighlightAtLeastOneChild;
	bAlwaysHighlightAtLeastOneChild = false;

	// Play sound only on highlight if it is set to true
	UnhighlightAllChildren();

	// Enable back
	bAlwaysHighlightAtLeastOneChild = bPreviousValue;

	const int32 OldIndex = GetSelectedIndex();
	InternalSetSelectedIndex(ChildIndex);
	const int32 NewIndex = GetSelectedIndex();

	HighlightSelectedChild(bPlaySound);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIUserWidgetArray::SelectFirstValidChild(bool bPlaySound)
{
	if (IsValidIndex(GetSelectedIndex()))
	{
		// Selected element is not active, unhighlight it and select the first active one if there is any
		if (USoUIUserWidgetBaseNavigation* SelectedChild = Cast<USoUIUserWidgetBaseNavigation>(GetChildAt(GetSelectedIndex())))
		{
			// Already valid, nothing to do
			//if (bIgnoreIfSelectedIsValid && SelectedChild->IsValidWidget())
			//	return false;

			// Just in case we could not find anything
			SelectedChild->Unhighlight();
		}
	}

	// Find a valid widget to select
	const int32 NumChildren = NumAllChildren();
	for (int32 Index = 0; Index < NumChildren; ++Index)
	{
		if (IsValidWidgetAt(Index))
		{
			// Found a valid child
			SetSelectedIndex(Index, bPlaySound);
			return true;
		}
	}

	// Something is wrong, no valid index found?
	InternalSetSelectedIndex(INDEX_NONE);
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoUIUserWidgetArray::SelectFirstActiveChild(int32 StartFrom)
{
	const int32 NumChildren = NumAllChildren();
	for (int32 Index = StartFrom; Index < NumChildren + StartFrom; Index++)
	{
		const int32 SafeIndex = Index % NumChildren;

		USoUIUserWidget* Child = GetChildAtAsSoUserWidget(SafeIndex);
		if (Child && Child->IsActive())
		{
			SetSelectedIndex(SafeIndex);
			return SafeIndex;
		}
	}

	SetSelectedIndex(INDEX_NONE);
	return INDEX_NONE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIUserWidgetArray::GetSelectedIndexChecked(int32& OutIndex) const
{
	OutIndex = GetSelectedIndex();
	return IsValidIndex(OutIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIUserWidgetArray::SetIsEnabledOnWidget(UUserWidget* Widget, bool bEnabled)
{
	if (!Widget)
		return false;

	Widget->SetIsEnabled(bEnabled);
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIUserWidgetArray::SetIsHighlightedOnWidget(UUserWidget* Widget, bool bInHighlighted, bool bPlaySound)
{
	if (!Widget)
		return false;

	if (USoUIUserWidgetBaseNavigation* UserWidgetNavigation = Cast<USoUIUserWidgetBaseNavigation>(Widget))
	{
		return UserWidgetNavigation->SetIsHighlighted(bInHighlighted, bPlaySound);
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIUserWidgetArray::SetIsHighlighted(bool bInHighlighted, bool bPlaySound)
{
	UE_LOG(
		LogSoUI,
		Verbose,
		TEXT("[%s] USoUIUserWidgetArray::SetIsHighlighted(bInHighlighted = %d, bPlaySound = %d)"),
		*GetName(), bInHighlighted, bPlaySound
	);
	UnhighlightAllChildren();
	if (bInHighlighted)
	{
		// Select first from Array
		if (!SelectFirstValidChild(bPlaySound))
		{
			// SelectedIndex is valid now
			HighlightSelectedChild(bPlaySound);
		}
	}

	return Super::SetIsHighlighted(bInHighlighted, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIUserWidgetArray::SetIsHighlightedOnChildAt(int32 ChildIndex, bool bHighlight, bool bPlaySound)
{
	return SetIsHighlightedOnWidget(GetChildAt(ChildIndex), bHighlight, bPlaySound);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIUserWidgetArray::SetIsActiveOnWidget(UUserWidget* Widget, bool bActive, bool bPlaySound)
{
	if (!Widget)
		return false;

	if (USoUIUserWidget* SoUserWidget = Cast<USoUIUserWidget>(Widget))
	{
		SoUserWidget->SetIsActive(bActive, bPlaySound);
		return true;
	}
	if (ThisClass* SoArray = Cast<ThisClass>(Widget))
	{
		SoArray->SetSelectedChildIsActive(bActive, bPlaySound);
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIUserWidgetArray::NavigateOnWidget(UUserWidget* Widget, ESoUICommand Command, bool bPlaySound)
{
	if (!Widget)
		return false;

	if (USoUIUserWidgetBaseNavigation* UserWidgetNavigation = Cast<USoUIUserWidgetBaseNavigation>(Widget))
		return UserWidgetNavigation->Navigate(Command, bPlaySound);

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIUserWidgetArray::IsWidgetEnabled(UUserWidget* Widget)
{
	return Widget ? Widget->GetIsEnabled() : false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIUserWidgetArray::IsWidgetValid(UUserWidget* Widget)
{
	if (!Widget)
		return false;

	if (USoUIUserWidgetBaseNavigation* UserWidgetNavigation = Cast<USoUIUserWidgetBaseNavigation>(Widget))
		return UserWidgetNavigation->IsValidWidget();

	return IsWidgetEnabled(Widget) && Widget->IsVisible();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIUserWidgetArray::IsWidgetActive(UUserWidget* Widget)
{
	if (!Widget)
		return false;

	if (USoUIUserWidget* SoUserWidget = Cast<USoUIUserWidget>(Widget))
	{
		return SoUserWidget->IsActive();
	}
	if (ThisClass* SoArray = Cast<ThisClass>(Widget))
	{
		return SoArray->IsSelectedChildActive();
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIUserWidgetArray::IsWidgetHighlighted(UUserWidget* Widget)
{
	if (!Widget)
		return false;

	if (USoUIUserWidget* SoUserWidget = Cast<USoUIUserWidget>(Widget))
	{
		return SoUserWidget->IsHighlighted();
	}
	if (ThisClass* SoArray = Cast<ThisClass>(Widget))
	{
		return SoArray->IsSelectedChildHighlighted();
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetArray::AddContainerChild(UUserWidget* Child)
{
	if (!Child)
		return;

	ContainerChildren.Add(Child);
	if (InternalCanUseContainer() && !Container->HasChild(Child))
		Container->AddChild(Child);

	const int32 AddedIndex = NumContainerChildren() - 1;
	SubscribeToChildEvents(Child, AddedIndex);
	UpdateWidgetOverrides(Child, AddedIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetArray::RemoveContainerChild(UUserWidget* Child)
{
	ContainerChildren.Remove(Child);
	if (InternalCanUseContainer() && Container->HasChild(Child))
		Container->RemoveChild(Child);

	UnSubscribeFromChildEvents(Child);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetArray::RemoveContainerChildAt(int32 ChildIndex)
{
	if (ContainerChildren.IsValidIndex(ChildIndex))
		RemoveContainerChild(ContainerChildren[ChildIndex]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetArray::EmptyContainerChildren(bool bContainer)
{
	// Unsubscribe
	if (Container && Container->GetChildrenCount() > 0)
	{
		// Container children
		for (int32 Index = 0; Index < Container->GetChildrenCount(); Index++)
		{
			UUserWidget* Widget = Cast<UUserWidget>(Container->GetChildAt(Index));
			if (!Widget)
				continue;

			UnSubscribeFromChildEvents(Widget);
		}
	}
	UnhighlightSelectedChild(false);

	ContainerChildren.Empty();

	if (bContainer && InternalCanUseContainer())
	{
		Container->ClearChildren();
		InvalidateLayoutAndVolatility();
	}
	SelectedIndex = INDEX_NONE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UUserWidget* USoUIUserWidgetArray::GetContainerChildAt(int32 ChildIndex) const
{
	return IsIndexForContainer(ChildIndex) ? ContainerChildren[ChildIndex] : nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetArray::HideContainer()
{
	if (InternalCanUseContainer())
		Container->SetVisibility(ESlateVisibility::Collapsed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetArray::ShowContainer()
{
	if (InternalCanUseContainer())
		Container->SetVisibility(ESlateVisibility::Visible);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetArray::AddExtraAttachedChild(UUserWidget* ExtraChild)
{
	if (!ExtraChild)
		return;

	ExtraAttachedChildren.Add(ExtraChild);

	const int32 AddedIndex = NumAllChildren() - 1;
	SubscribeToChildEvents(ExtraChild, AddedIndex);
	UpdateWidgetOverrides(ExtraChild, AddedIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetArray::EmptyExtraAttachedChildren()
{
	// unsubscribe
	for (UUserWidget* Widget : ExtraAttachedChildren)
		UnSubscribeFromChildEvents(Widget);

	ExtraAttachedChildren.Empty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UUserWidget* USoUIUserWidgetArray::GetExtraAttachedChildAt(int32 ChildIndex) const
{
	return IsIndexForExtraAttachedChildren(ChildIndex) ? ExtraAttachedChildren[ConvertAllChildIndexToExtraAttachedChildIndex(ChildIndex)] : nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIUserWidgetArray::IsValidIndex(int32 ChildIndex) const
{
	// Not in range
	const int32 Num = NumAllChildren();
	if (ChildIndex < 0 || ChildIndex >= Num)
		return false;

	return IsIndexForContainer(ChildIndex) || IsIndexForExtraAttachedChildren(ChildIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UUserWidget* USoUIUserWidgetArray::GetChildAt(int32 ChildIndex) const
{
	// Not in range
	if (!IsValidIndex(ChildIndex))
		return nullptr;

	// In the normal children array
	UUserWidget* Child = GetContainerChildAt(ChildIndex);
	return Child ? Child : GetExtraAttachedChildAt(ChildIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUIUserWidget* USoUIUserWidgetArray::GetChildAtAsSoUserWidget(int32 ChildIndex) const
{
	return Cast<USoUIUserWidget>(GetChildAt(ChildIndex));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUIUserWidgetArray* USoUIUserWidgetArray::GetChildAtAsSoUserWidgetArray(int32 ChildIndex) const
{
	return Cast<ThisClass>(GetChildAt(ChildIndex));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetArray::SubscribeToChildEvents(UUserWidget* Child, int32 ChildIndex)
{
	if (!Child)
		return;

	auto* UserWidgetNavigation = Cast<USoUIUserWidgetBaseNavigation>(Child);
	if (!UserWidgetNavigation)
		return;

		const uint32 Key = Child->GetUniqueID();

	// Listen to enabled change events
	if (!DelegatesEnabledChangedMap.Contains(Key))
	{
		const FDelegateHandle NewHandle = UserWidgetNavigation->OnEnabledChangedEvent().AddLambda([this, ChildIndex](bool bEnabled)
		{
			OnChildEnabledChanged(ChildIndex, bEnabled);
		});

		DelegatesEnabledChangedMap.Add(Key);
	}

	// Listen to highlight change events
	if (!DelegatesHighlightChangedMap.Contains(Key))
	{
		const FDelegateHandle NewHandle = UserWidgetNavigation->OnHighlightChangedEvent().AddLambda([this, ChildIndex](bool bHighlight)
		{
			OnChildHighlightChanged(ChildIndex, bHighlight);
		});

		DelegatesHighlightChangedMap.Add(Key);
	}

	// Listen to highlight request change event
	if (bListenToChildHighlightRequest && !DelegatesHighlightRequestChangeMap.Contains(Key))
	{
		const FDelegateHandle NewHandle = UserWidgetNavigation->OnHighlightRequestChangeEvent().AddLambda([this, ChildIndex](bool bHighlight)
		{
			OnRequestChildHighlightChange(ChildIndex, bHighlight);
		});

		DelegatesHighlightRequestChangeMap.Add(Key, NewHandle);
	}

	// Listen to pressed event
	if (bListenToChildPressedRequest && !DelegatesPressedRequestMap.Contains(Key))
	{
		const FDelegateHandle NewHandle = UserWidgetNavigation->OnPressedEvent().AddUObject(this, &ThisClass::OnRequestChildPressed, ChildIndex);
		DelegatesPressedRequestMap.Add(Key, NewHandle);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetArray::UnSubscribeFromChildEvents(UUserWidget* Child)
{
	if (!Child)
		return;

	auto* UserWidgetNavigation = Cast<USoUIUserWidgetBaseNavigation>(Child);
	if (!UserWidgetNavigation)
		return;


	const uint32 Key = Child->GetUniqueID();

	if (DelegatesEnabledChangedMap.Contains(Key))
	{
		UserWidgetNavigation->OnEnabledChangedEvent().Remove(DelegatesEnabledChangedMap.FindChecked(Key));
		DelegatesEnabledChangedMap.Remove(Key);
	}
	if (DelegatesHighlightChangedMap.Contains(Key))
	{
		UserWidgetNavigation->OnHighlightChangedEvent().Remove(DelegatesHighlightChangedMap.FindChecked(Key));
		DelegatesHighlightChangedMap.Remove(Key);
	}
	if (DelegatesHighlightRequestChangeMap.Contains(Key))
	{
		UserWidgetNavigation->OnHighlightRequestChangeEvent().Remove(DelegatesHighlightRequestChangeMap.FindChecked(Key));
		DelegatesHighlightRequestChangeMap.Remove(Key);
	}
	if (DelegatesPressedRequestMap.Contains(Key))
	{
		UserWidgetNavigation->OnPressedEvent().Remove(DelegatesPressedRequestMap.FindChecked(Key));
		DelegatesPressedRequestMap.Remove(Key);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetArray::OnRequestChildHighlightChange(int32 ChildIndex, bool bHighlight)
{
	if (!GetChildAt(ChildIndex))
		return;

	// if (USoUIUserWidget* Widget = GetChildAtAsSoUserWidget(ChildIndex))
	// {
	// 	if (Widget->IsHandlingOwnMouseEvents())
	// 		return;
	// }

	UE_LOG(
		LogSoUI,
		Verbose,
		TEXT("[%s] USoUIUserWidgetArray::OnRequestChildHighlightChange(ChildIndex = %d, bHighlight = %d)"),
		*GetName(), ChildIndex,bHighlight
	);

	if (bHighlight)
	{
		if (bHighlightSelectsChild)
		{
			SetSelectedIndex(ChildIndex, true);
		}
		else
		{
			UnhighlightAllChildren();
			HighlightChildAt(ChildIndex, true);
		}
	}
	else
	{
		UnhighlightChildAt(ChildIndex, false);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetArray::OnChildHighlightChanged(int32 ChildIndex, bool bHighlight)
{
	UE_LOG(
		LogSoUI,
		Verbose,
		TEXT("[%s] USoUIUserWidgetArray::OnChildHighlightChanged(ChildIndex = %d, bHighlight = %d)"),
		*GetName(), ChildIndex, bHighlight
	);

	ReceiveOnChildHighlightChanged(ChildIndex, bHighlight);
	ChildHighlightChangedEvent.Broadcast(ChildIndex, bHighlight);
	EnsureHighlights();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetArray::OnChildEnabledChanged(int32 ChildIndex, bool bEnabled)
{
	UE_LOG(
		LogSoUI,
		Verbose,
		TEXT("[%s] USoUIUserWidgetArray::OnChildEnabledChanged(ChildIndex = %d, bEnabled = %d)"),
		*GetName(), ChildIndex, bEnabled
	);

	ReceiveOnChildEnabledChanged(ChildIndex, bEnabled);
	ChildEnabledChangedEvent.Broadcast(ChildIndex, bEnabled);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetArray::OnRequestChildPressed(int32 ChildIndex)
{
	if (!GetChildAt(ChildIndex))
		return;

	// Widget is handling its own events
	if (USoUIUserWidget* Widget = GetChildAtAsSoUserWidget(ChildIndex))
	{
		if (Widget->IsHandlingOwnMouseEvents())
			return;
	}

	if (bHighlightSelectsChild)
	{
		OnRequestChildHighlightChange(ChildIndex, true);
	}
	else
	{
		SetSelectedIndex(ChildIndex, true);
	}
	NavigateOnPressed(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetArray::UpdateAllOverrides()
{
	MapOverContainerChildren([&](int32 Index, UUserWidget* Widget)
	{
		UpdateWidgetOverrides(Widget, Index);
	});

	// Extra children
	for (int32 Index = 0; Index < NumExtraAttachedChildren(); Index++)
	{
		UUserWidget* Widget = ExtraAttachedChildren[Index];
		if (!Widget)
			continue;

		// Normalize index to all children indices
		UpdateWidgetOverrides(Widget, NumContainerChildren() + Index);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetArray::UpdateWidgetOverrides(UUserWidget* Widget, int32 ChildIndex)
{
	UpdateOverrideCustomPadding(Widget);
	UpdateOverridesSFX(Widget);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetArray::UpdateOverridesSFX(UUserWidget* Widget)
{
	if (!bSFXOverrideIndividualChildren)
		return;

	if (USoUIUserWidget* UserWidget = Cast<USoUIUserWidget>(Widget))
	{
		if (SFXChildSelectionChanged)
			UserWidget->SetSFXHighlightChanged(SFXChildSelectionChanged);

		if (SFXChildPressed)
			UserWidget->SetSFXOnPressed(SFXChildPressed);
	}
	else if (ThisClass* SoArray = Cast<ThisClass>(Widget))
	{
		if (SFXChildSelectionChanged)
			SoArray->SetSFXChildSelectionChanged(SFXChildSelectionChanged);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetArray::UpdateOverrideCustomPadding(UUserWidget* Widget)
{
	if (!bUseCustomPadding)
		return;

	Widget->SetPadding(PaddingForEachChildren);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetArray::MapOverContainerChildren(TFunction<void(int32, UUserWidget*)> CallbackLoopBody)
{
	// TODO handle case when ContainerChildren != Container.Children
	// used in scrolling array?

	if (NumContainerChildren() > 0)
	{
		// Children
		for (int32 Index = 0; Index < NumContainerChildren(); Index++)
		{
			UUserWidget* Widget = GetContainerChildAt(Index);
			if (!Widget)
				continue;

			CallbackLoopBody(Index, Widget);
		}
	}
	else if (Container && Container->GetChildrenCount() > 0)
	{
		// Container children
		for (int32 Index = 0; Index < Container->GetChildrenCount(); Index++)
		{
			UUserWidget* Widget = Cast<UUserWidget>(Container->GetChildAt(Index));
			if (!Widget)
				continue;

			CallbackLoopBody(Index, Widget);
		}
	}
}

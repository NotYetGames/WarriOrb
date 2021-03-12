// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUIUserWidgetBaseNavigation.h"
#include "Basic/SoAudioManager.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetBaseNavigation::SynchronizeProperties()
{
	Super::SynchronizeProperties();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetBaseNavigation::NativeConstruct()
{
	Super::NativeConstruct();

	// Call our CPP
	StateChangedEvent.AddDynamic(this, &ThisClass::OnProxyStateChangedEvent);
	PressedEvent.AddDynamic(this, &ThisClass::OnProxyPressedEvent);
	HighlightChangedEvent.AddDynamic(this, &ThisClass::OnProxyHighlightChangedEvent);
	EnabledChangedEvent.AddDynamic(this, &ThisClass::OnProxyEnabledChangedEvent);

	bIsConstructed = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetBaseNavigation::NativeDestruct()
{
	StateChangedEvent.RemoveDynamic(this, &ThisClass::OnProxyStateChangedEvent);
	PressedEvent.RemoveDynamic(this, &ThisClass::OnProxyPressedEvent);
	HighlightChangedEvent.RemoveDynamic(this, &ThisClass::OnProxyHighlightChangedEvent);
	EnabledChangedEvent.RemoveDynamic(this, &ThisClass::OnProxyEnabledChangedEvent);

	Super::NativeDestruct();
	bIsConstructed = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetBaseNavigation::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Give Event to CPP
	OnUIEventType(ESoUIEventType::MouseEnter);

	// Give event to BP
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetBaseNavigation::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	// Give Event to CPP
	OnUIEventType(ESoUIEventType::MouseLeave);

	// Give event to BP
	Super::NativeOnMouseLeave(InMouseEvent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FReply USoUIUserWidgetBaseNavigation::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Give Event to CPP
	const FReply CPPReply = OnUIEventType(ESoUIEventType::MouseButtonDown);

	// Give event to BP
	const FReply BPReply = Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

	return BPReply.IsEventHandled() ? BPReply : CPPReply;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FReply USoUIUserWidgetBaseNavigation::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Give Event to CPP
	const FReply CPPReply = OnUIEventType(ESoUIEventType::MouseButtonUp);

	// Give event to BP
	const FReply BPReply = Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);

	return BPReply.IsEventHandled() ? BPReply : CPPReply;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIUserWidgetBaseNavigation::Navigate(ESoUICommand Command, bool bPlaySound)
{
	PreNavigateEvent.Broadcast();

	bool bStatus = false;
	if (Command == ESoUICommand::EUC_MainMenuEnter)
	{
		bStatus = NavigateOnPressed(bPlaySound);
	}

	PostNavigateEvent.Broadcast(bStatus);
	return bStatus;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIUserWidgetBaseNavigation::NavigateOnPressed(bool bPlaySound)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FReply USoUIUserWidgetBaseNavigation::OnUIEventType(ESoUIEventType EventType)
{
	switch (EventType)
	{
	case ESoUIEventType::None:
		break;
	case ESoUIEventType::MouseEnter:
		if (bHandleOwnMouseEvents)
			SetIsHighlighted(true, true);
		else
			HighlightRequestChangeEvent.Broadcast(true);

		return FReply::Handled();

	case ESoUIEventType::MouseLeave:
		if (bHandleOwnMouseEvents)
			SetIsHighlighted(false, false);
		else
			HighlightRequestChangeEvent.Broadcast(false);

		return FReply::Handled();

	case ESoUIEventType::MouseButtonDown:
		SetIsPressed(true);
		if (bHandleOwnMouseEvents)
			NavigateOnPressed(true);

		break;

	case ESoUIEventType::MouseButtonUp:
		SetIsPressed(false);
		break;
	default:
		break;
	}

	return FReply::Unhandled();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIUserWidgetBaseNavigation::SetIsHighlighted(bool bInHighlighted, bool bPlaySound)
{
	if (bIsHighlighted != bInHighlighted)
	{
		if (bPlaySound)
			USoAudioManager::PlaySound2D(this, SFXHighlightChanged);

		bIsHighlighted = bInHighlighted;
		UE_LOG(
			LogSoUI,
			Verbose,
			TEXT("[%s] USoUIUserWidgetBaseNavigation::SetIsHighlighted(bIsHighlighted = %d)"),
			*GetName(), bIsHighlighted
		);
		OnHighlightChanged(bIsHighlighted);
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetBaseNavigation::SetIsEnabled(bool bInIsEnabled)
{
	if (GetIsEnabled() != bInIsEnabled)
	{
		Super::SetIsEnabled(bInIsEnabled);
		OnEnabledChanged(bInIsEnabled);
	}
}

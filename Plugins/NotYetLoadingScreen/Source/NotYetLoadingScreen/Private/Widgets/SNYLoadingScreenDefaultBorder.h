// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"


class SNYLoadingScreenDefaultBorder : public SBorder
{
	typedef SBorder Super;
public:

	SLATE_BEGIN_ARGS(SNYLoadingScreenDefaultBorder)
		: _OnKeyDown()
	{}

		SLATE_EVENT(FPointerEventHandler, OnMouseButtonDown)
		SLATE_EVENT(FOnKeyDown, OnKeyDown)
		SLATE_DEFAULT_SLOT(FArguments, Content)

	SLATE_END_ARGS()

	/**
	* Construct this widget
	*
	* @param	InArgs	The declaration data for this widget
	*/
	void Construct(const FArguments& InArgs)
	{
		OnKeyDownHandler = InArgs._OnKeyDown;
		bCanSupportFocus = true;

		SBorder::Construct(SBorder::FArguments()
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("BlackBrush")))
			.OnMouseButtonDown(InArgs._OnMouseButtonDown)
			.Padding(0)[InArgs._Content.Widget]);

	}

	/**
	* Set the handler to be invoked when the user presses a key.
	*
	* @param InHandler   Method to execute when the user presses a key
	*/
	void SetOnOnKeyDown(const FOnKeyDown& InHandler)
	{
		OnKeyDownHandler = InHandler;
	}

	/**
	* Overrides SWidget::OnKeyDown()
	* executes OnKeyDownHandler if it is bound
	*/
	FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
	{
		if (OnKeyDownHandler.IsBound())
		{
			// If a handler is assigned, call it.
			return OnKeyDownHandler.Execute(MyGeometry, InKeyEvent);
		}
		return SBorder::OnKeyDown(MyGeometry, InKeyEvent);
	}

	/**
	* Overrides SWidget::SupportsKeyboardFocus()
	* Must support keyboard focus to accept OnKeyDown events
	*/
	bool SupportsKeyboardFocus() const override
	{
		return true;
	}

	/**
	 * Called when focus is given to this widget.  This event does not bubble.
	 *
	 * @param MyGeometry The Geometry of the widget receiving the event
	 * @param  InFocusEvent  The FocusEvent
	 * @return  Returns whether the event was handled, along with other possible actions
	 */
	FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override
	{
		return FReply::Handled();
	}

	/**
	 * Called when this widget loses focus.  This event does not bubble.
	 *
	 * @param InFocusEvent The FocusEvent
	 */
	void OnFocusLost(const FFocusEvent& InFocusEvent) override
	{
		Super::OnFocusLost(InFocusEvent);
	}

	/** Called whenever a focus path is changing on all the widgets within the old and new focus paths */
	void OnFocusChanging(const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath, const FFocusEvent& InFocusEvent) override
	{
		Super::OnFocusChanging(PreviousFocusPath, NewWidgetPath, InFocusEvent);
	}

protected:
	FOnKeyDown OnKeyDownHandler;
};

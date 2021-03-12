// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoUIInputKeySelector.h"

#include "Engine/Font.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/FrameworkObjectVersion.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "SSoInputKeySelector.h"
#include "Localization/SoLocalization.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUIInputKeySelector::USoUIInputKeySelector(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	const SSoInputKeySelector::FArguments InputKeySelectorDefaults;
	WidgetStyle = *InputKeySelectorDefaults._ButtonStyle;
	TextStyle = *InputKeySelectorDefaults._TextStyle;
	KeySelectionText = InputKeySelectorDefaults._KeySelectionText;
	NoKeySpecifiedText = InputKeySelectorDefaults._NoKeySpecifiedText;
	SelectedKey = InputKeySelectorDefaults._SelectedKey.Get();
	bAllowModifierKeysInSelection = InputKeySelectorDefaults._AllowModifierKeysInSelection;
	AllowedInputType = InputKeySelectorDefaults._AllowedInputType;

	EscapeKeys.AddUnique(EKeys::Gamepad_Special_Right); // In most (if not all) cases this is going to be the menu button

	if (!IsRunningDedicatedServer())
	{
		static ConstructorHelpers::FObjectFinder<UFont> RobotoFontObj(TEXT("/Engine/EngineFonts/Roboto"));
		TextStyle.Font = FSlateFontInfo(RobotoFontObj.Object, 24, FName("Bold"));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIInputKeySelector::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar.UsingCustomVersion(FFrameworkObjectVersion::GUID);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIInputKeySelector::PostLoad()
{
	Super::PostLoad();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIInputKeySelector::SetSelectedKey(const FInputChord& InSelectedKey)
{
	if (MyInputKeySelector.IsValid())
	{
		MyInputKeySelector->SetSelectedKey(InSelectedKey);
	}
	SelectedKey = InSelectedKey;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIInputKeySelector::SetKeySelectionText(FText InKeySelectionText)
{
	if (MyInputKeySelector.IsValid())
	{
		MyInputKeySelector->SetKeySelectionText(InKeySelectionText);
	}
	KeySelectionText = MoveTemp(InKeySelectionText);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIInputKeySelector::SetNoKeySpecifiedText(FText InNoKeySpecifiedText)
{
	if (MyInputKeySelector.IsValid())
	{
		MyInputKeySelector->SetNoKeySpecifiedText(InNoKeySpecifiedText);
	}
	NoKeySpecifiedText = MoveTemp(InNoKeySpecifiedText);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIInputKeySelector::SetPreviousFocusedWidget(TSharedPtr<SWidget> InWidget)
{
	if (MyInputKeySelector.IsValid())
	{
		MyInputKeySelector->SetPreviousFocusedWidget(InWidget);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIInputKeySelector::SetAllowModifierKeysInSelection(bool bInAllowModifierKeys)
{
	if (MyInputKeySelector.IsValid())
	{
		MyInputKeySelector->SetAllowModifierKeysInSelection(bInAllowModifierKeys);
	}
	bAllowModifierKeysInSelection = bInAllowModifierKeys;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIInputKeySelector::SetAllowedInputType(ESoInputKeySelectorDeviceType NewType)
{
	if (MyInputKeySelector.IsValid())
	{
		MyInputKeySelector->SetAllowedInputType(NewType);
	}
	AllowedInputType = NewType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIInputKeySelector::IsSelectingKey() const
{
	return MyInputKeySelector.IsValid() ? MyInputKeySelector->IsSelectingKey() : false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIInputKeySelector::SetIsSelectingKey(bool bInIsSelectingKey)
{
	UE_LOG(LogSoInputSelector, Verbose, TEXT("USoUIInputKeySelector::SetIsSelectingKey = %d"), bInIsSelectingKey);
	if (MyInputKeySelector.IsValid())
		MyInputKeySelector->SetIsSelectingKey(bInIsSelectingKey);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIInputKeySelector::SetButtonStyle(const FButtonStyle* InButtonStyle)
{
	if (MyInputKeySelector.IsValid())
	{
		MyInputKeySelector->SetButtonStyle(InButtonStyle);
	}
	WidgetStyle = *InButtonStyle;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIInputKeySelector::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (MyInputKeySelector.IsValid())
	{
		MyInputKeySelector->SetSelectedKey(SelectedKey);
		MyInputKeySelector->SetMargin(Margin);
		MyInputKeySelector->SetButtonStyle(&WidgetStyle);
		MyInputKeySelector->SetTextStyle(&TextStyle);
		MyInputKeySelector->SetKeySelectionText(KeySelectionText);
		MyInputKeySelector->SetAllowModifierKeysInSelection(bAllowModifierKeysInSelection);
		MyInputKeySelector->SetAllowedInputType(AllowedInputType);
		MyInputKeySelector->SetEscapeKeys(EscapeKeys);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIInputKeySelector::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyInputKeySelector.Reset();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TSharedRef<SWidget> USoUIInputKeySelector::RebuildWidget()
{
	MyInputKeySelector = SNew(SSoInputKeySelector)
		.SelectedKey(SelectedKey)
		.Margin(Margin)
		.ButtonStyle(&WidgetStyle)
		.TextStyle(&TextStyle)
		.KeySelectionText(KeySelectionText)
		.AllowModifierKeysInSelection(bAllowModifierKeysInSelection)
		.AllowedInputType(AllowedInputType)
		.EscapeCancelsSelection(true)
		.IsButtonFocusable(true)
		.EscapeKeys(EscapeKeys)
		.NoKeySpecifiedText(FROM_STRING_TABLE_UI("empty"))
		.OnKeySelectedEvent(BIND_UOBJECT_DELEGATE(SSoInputKeySelector::FSoKeySelectedEvent, HandleKeySelected))
		.OnIsSelectingKeyChangedEvent(BIND_UOBJECT_DELEGATE(SSoInputKeySelector::FSoIsSelectingKeyChangedEvent, HandleIsSelectingKeyChanged));

	return MyInputKeySelector.ToSharedRef();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIInputKeySelector::HandleKeySelected(const FInputChord& InSelectedKey)
{
	SelectedKey = InSelectedKey;
	KeySelectedEvent.Broadcast(SelectedKey);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIInputKeySelector::HandleIsSelectingKeyChanged()
{
	IsSelectingKeyChangedEvent.Broadcast(IsSelectingKey());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIInputKeySelector::SetTextBlockVisibility(ESlateVisibility InVisibility)
{
	if (MyInputKeySelector.IsValid())
	{
		const EVisibility SlateVisibility = ConvertSerializedVisibilityToRuntime(InVisibility);
		MyInputKeySelector->SetTextBlockVisibility(SlateVisibility);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIInputKeySelector::SetEscapeKeys(const TArray<FKey>& InEscapeKeys)
{
	if (MyInputKeySelector.IsValid())
	{
		MyInputKeySelector->SetEscapeKeys(InEscapeKeys);
	}
	EscapeKeys = InEscapeKeys;
}

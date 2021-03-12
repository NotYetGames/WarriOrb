// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoUIConfirmPanel.h"


#include "Commands/SoUICommandTooltipArray.h"
#include "Buttons/SoUIButtonArray.h"
#include "Buttons/SoUIButtonImageArray.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmPanel::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	RefreshButtonsArray();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmPanel::Open_Implementation(bool bOpen)
{
	if (bOpened == bOpen)
		return;

	bOpened = bOpen;
	SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (ButtonsArray)
		ButtonsArray->Highlight();

	OpenChangedEvent.Broadcast(bOpened);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUIButtonArray* USoUIConfirmPanel::GetButtonArray() const
{
	return ButtonsArray;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUIButtonImageArray* USoUIConfirmPanel::GetButtonArrayAsButtonImagesArray() const
{
	return Cast<USoUIButtonImageArray>(GetButtonArray());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmPanel::UpdateEnsureOnlyTheseCommandsExist(const TArray<FSoUITextCommandPair> CommandsToBeDisplayed)
{
	USoUIButtonImageArray* ImageArray = GetButtonArrayAsButtonImagesArray();
	if (!ImageArray)
	{
		UE_LOG(LogSoUI, Error, TEXT("EnsureOnlyTheseCommandsExist: Not a USoUIButtonImageArray, in the widget = %s"), *GetPathName());
		return;
	}

	ImageArray->UpdateEnsureOnlyTheseCommandsExist(CommandsToBeDisplayed);
}

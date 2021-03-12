// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.


#include "SoUICredits.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Components/RichTextBlock.h"

#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/SoAudioManager.h"
#include "Basic/Helpers/SoDateTimeHelper.h"
#include "UI/General/SoUIPressAndHoldConfirmation.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICredits::NativeConstruct()
{
	Super::NativeConstruct();

	if (SkipButton)
	{
		SkipButton->OnConfirmed().AddDynamic(this, &USoUICredits::Skip);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICredits::NativeDestruct()
{
	Super::NativeDestruct();

	if (SkipButton)
		SkipButton->OnConfirmed().RemoveDynamic(this, &USoUICredits::Skip);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICredits::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	switch (ActiveState)
	{
		case ESoUICreditsState::FadeIn:
			if (!IsAnimationPlaying(FadeAnim))
			{
				StartScroll();
			}
			break;

		case ESoUICreditsState::FadeOut:
			if (FadeOutCounter > 0.0f)
			{
				FadeOutCounter -= InDeltaTime;
				if (FadeOutCounter <= 0.0f)
				{
					FadeOutFinished.Broadcast();
					USoAudioManager::Get(this).SetMusic(MusicOnExit, true, -1.0f);
				}
			}

			if (!IsAnimationPlaying(FadeAnim))
			{
				Hide();
				if (FadeOutCounter > 0.0f)
					FadeOutFinished.Broadcast();
			}
			break;

		case ESoUICreditsState::Inactive:
			SetVisibility(ESlateVisibility::Collapsed);
			break;

		case ESoUICreditsState::Scrolling:
		{
			Counter -= InDeltaTime;
			if (Counter <= 0.0f && !IsAnimationPlaying(ScrollAnim))
			{
				if (bUseFadeOut)
					FadeOut();

				CreditsFinished.Broadcast();
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICredits::StartScroll()
{
	SetVisibility(ESlateVisibility::Visible);
	// the value the fade might ruin
	SetRenderOpacity(1.0f);

	ActiveState = ESoUICreditsState::Scrolling;
	StartScrollBP();
	Counter = USoDateTimeHelper::NormalizeTime(Duration);

	if (SkipButton->IsConfirmed())
		SkipButton->Reinitialize();
	SkipButton->SetConfirmationEnabled(true);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICredits::FadeIn(bool bInUseFadeOut)
{
	if (ActiveState != ESoUICreditsState::Inactive)
	{
		StopAllAnimations();
	}

	bUseFadeOut = bInUseFadeOut;
	SetVisibility(ESlateVisibility::Visible);
	FadeInBP();
	ActiveState = ESoUICreditsState::FadeIn;

	USoAudioManager::Get(this).SetMusic(Music, true, -1.0f);

	SkipButton->Reinitialize();
	SkipButton->SetConfirmationEnabled(false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICredits::FadeOut()
{
	if (ActiveState == ESoUICreditsState::FadeOut)
	{
		return;
	}

	// BP call should be in previous state so we know if we might need to switch anim dir (if fade in)
	FadeOutBP();
	ActiveState = ESoUICreditsState::FadeOut;
	FadeOutCounter = USoDateTimeHelper::NormalizeTime(0.5f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICredits::HandleCommand(ESoUICommand Command)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICredits::Hide()
{
	StopAllAnimations();
	ActiveState = ESoUICreditsState::Inactive;
	SetVisibility(ESlateVisibility::Collapsed);
	USoAudioManager::Get(this).SetMusic(MusicOnExit, true, -1.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICredits::Skip()
{
	SkipButton->SetConfirmationEnabled(false);
	if (bUseFadeOut)
		FadeOut();
	CreditsFinished.Broadcast();
}

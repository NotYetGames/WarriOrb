// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUIEnding.h"

#include "Components/CanvasPanel.h"
#include "Components/RichTextBlock.h"
#include "Components/Image.h"

#include "Materials/MaterialInstanceDynamic.h"

#include "Basic/Helpers/SoStringHelper.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/Helpers/SoDateTimeHelper.h"
#include "Character/SoCharacter.h"
#include "Character/SoPlayerProgress.h"
#include "Basic/SoAudioManager.h"
#include "Basic/SoGameInstance.h"
#include "UI/SoUICredits.h"
#include "SaveFiles/SoWorldState.h"
#include "Levels/SoLevelHelper.h"

#if WITH_EDITOR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIEnding::PreSave(const ITargetPlatform* TargetPlatform)
{
	UpdateLocalizedFields();
	Super::PreSave(TargetPlatform);
}
#endif // WITH_EDITOR

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIEnding::UpdateLocalizedFields()
{
	const FString ObjectName = USoStringHelper::GetObjectBaseName(this);
	const FString TextNamespace(TEXT("EndScreen"));

	for (int32 EntryIndex = 0; EntryIndex < EndingLogic.Num(); EntryIndex++)
	{
		auto& Entry = EndingLogic[EntryIndex];
		for (int32 TextIndex = 0; TextIndex < EndingLogic[EntryIndex].Texts.Num(); TextIndex++)
		{
			auto& Text = Entry.Texts[TextIndex];
			if (Text.ShouldGatherForLocalization())
			{
				Text = FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText(
					*Text.ToString(),
					*TextNamespace,
					*(ObjectName + "_entry_" + FString::FromInt(EntryIndex) + "_text_" + FString::FromInt(TextIndex))
				);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIEnding::NativeConstruct()
{
	Super::NativeConstruct();

	HideAllTextBlocks();

	SetVisibility(ESlateVisibility::Collapsed);

#if WITH_EDITOR
	const bool bCanBeVisible = true;
#else
	const bool bCanBeVisible = (USoLevelHelper::GetChapterNameFromObject(this) == FName("SoEnding"));
#endif

	for (FSoUIEndingEntry& Entry : EndingLogic)
	{
		if (UImage* Img = GetImage(Entry.ImageName))
		{
			if (UTexture* Texture = Cast<UTexture>(Img->Brush.GetResourceObject()))
			{
				if (bCanBeVisible)
					TextureMap.Add(Entry.ImageName, Texture);

				if (const FSoUIImageFadeData* FadeData = ImageFadeData.Find(Entry.ImageName))
					Img->SetBrushFromMaterial(FadeData->FadeInMaterial);
				else
					Img->SetBrushFromMaterial(ImgMaterial);

				if (UMaterialInstanceDynamic* DynamicMaterial = Img->GetDynamicMaterial())
				{
					DynamicMaterial->SetTextureParameterValue(FName("Texture"), bCanBeVisible ? Texture : nullptr);
					DynamicMaterial->SetScalarParameterValue(FName("FadeStr"), 0.0f);
				}
			}
		}
	}

	if (Credits != nullptr)
		Credits->OnCreditsFinished().AddDynamic(this, &USoUIEnding::OnCreditsFinished);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIEnding::NativeDestruct()
{
	if (Credits != nullptr)
		Credits->OnCreditsFinished().RemoveDynamic(this, &USoUIEnding::OnCreditsFinished);

	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIEnding::SetInGameActivityEnabled_Implementation(UObject* Source, bool bEnable)
{
	if (bEnable)
	{
		SetVisibility(ESlateVisibility::Visible);
		bShouldBeOpen = true;
		EndingUIState = ESoEndingUIState::Idle;

		ActiveSequenceIndex = 0;
		RefreshPreview(0.0f);

		ActiveSequenceSubIndex = 0;
		FadeCounter = 0.0f;
		bFadeImage = true;
		EndingUIState = ESoEndingUIState::FadeIn;
		StartFadeInBP();
		if (UImage* Img = GetImage(EndingLogic[0].ImageName))
		{
			Img->SetVisibility(ESlateVisibility::Visible);
			// SetImageRenderOpacity(Img, 0.0f);
			SetImageMaterialParams(Img, EndingLogic[0].ImageName, true);
		}

		if (EndingLogic.IsValidIndex(0))
			USoAudioManager::Get(this).SetMusic(EndingLogic[0].Music, true, 0.0f);

		ESoDifficulty GameDifficulty = FSoWorldState::Get().GetGameDifficulty();

		// Unlock speedrun achievement
		if (GameDifficulty == ESoDifficulty::Intended || GameDifficulty == ESoDifficulty::Insane)
		{
			if (ASoCharacter* SoChar = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
			{
				if (USoPlayerProgress* PlayerProgress = SoChar->GetPlayerProgress())
				{
					// 3 Hours
					const float ThresholdSeconds = 180.0f * 60.0f;
					if (PlayerProgress->GetCurrentPlayTimeSeconds() <= ThresholdSeconds)
					{
						static const FName SpeedrunnerAchievementName = FName("A_Speedrunner");
						USoAchievementManager::Get(this).UnlockAchievement(this, SpeedrunnerAchievementName);
					}
				}
			}

		}


		// Unlock insane achievement
		if (GameDifficulty == ESoDifficulty::Insane)
		{
			static const FName InsaneAchievementName = FName("A_Insane");
			USoAchievementManager::Get(this).UnlockAchievement(this, InsaneAchievementName);
		}

	}
	else
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}

	return bEnable;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIEnding::HandleCommand_Implementation(ESoUICommand Command)
{
	if (EndingUIState == ESoEndingUIState::Idle && Command == ESoUICommand::EUC_MainMenuEnter)
	{
		if (EndingLogic.IsValidIndex(ActiveSequenceIndex))
		{
			if (ActiveSequenceSubIndex == 1 && EndingLogic[ActiveSequenceIndex].Texts.Num() > 1)
			{
				EndingUIState = ESoEndingUIState::EntryUpdate;
				FadeCounter = FadeInDurationFirstText;
				PlayVO(ActiveSequenceIndex, ActiveSequenceSubIndex);
			}
			else if (EndingLogic.IsValidIndex(ActiveSequenceIndex + 1))
			{
				bFadeImage = EndingLogic[ActiveSequenceIndex].ImageName != EndingLogic[ActiveSequenceIndex + 1].ImageName &&
					EndingLogic[ActiveSequenceIndex].ImageName != NAME_None;
				EndingUIState = ESoEndingUIState::EntryFadeOut;
				FadeCounter = 0.0f;

				if (bFadeImage)
					SetImageMaterialParams(GetImage(EndingLogic[ActiveSequenceIndex].ImageName), EndingLogic[ActiveSequenceIndex].ImageName, false);

				if (EndingLogic[ActiveSequenceIndex + 1].Music != nullptr && EndingLogic[ActiveSequenceIndex + 1].Music != EndingLogic[ActiveSequenceIndex].Music)
					USoAudioManager::Get(this).SetMusic(EndingLogic[ActiveSequenceIndex + 1].Music, true, GetFadeOutDuration());
			}
			else
			{
				EndingUIState = ESoEndingUIState::Finished;
				OnFinished();
			}
		}
		else
		{
			EndingUIState = ESoEndingUIState::Finished;
			OnFinished();
		}
	}
	else if (EndingUIState == ESoEndingUIState::Finished && Command != ESoUICommand::EUC_MainMenuBack)
	{
		if (Credits)
		{
			Credits->HandleCommand(Command);
			return true;
		}
	}

	// let the menu open on escape
	return Command != ESoUICommand::EUC_MainMenuBack;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIEnding::Update_Implementation(float DeltaSeconds)
{
	switch (EndingUIState)
	{
		case ESoEndingUIState::FadeIn:
			if (!IsAnyAnimationPlaying())
				EndingUIState = ESoEndingUIState::EntryFadeIn;
			break;

		case ESoEndingUIState::EntryFadeOut:
		{
			FadeCounter += DeltaSeconds;

			if (FadeCounter <= GetFadeOutDuration())
			{
				const float NewRenderOpacity = USoMathHelper::Interpolate(1.0f,
																		  0.0f,
																		  FMath::Clamp(FadeCounter / GetFadeOutDuration(), 0.0f, 1.0f),
																		  bFadeImage ? FadeOutMethodImage : FadeOutMethod);

				if (bFadeImage)
					SetImageRenderOpacity(ActiveSequenceIndex, NewRenderOpacity);

				for (int32 i = 0; i < EndingLogic[ActiveSequenceIndex].Texts.Num(); ++i)
					SetTextBoxRenderOpacity(ActiveSequenceIndex, i, NewRenderOpacity);

				if (URichTextBlock* TB = GetTextBlock(EndingLogic[ActiveSequenceIndex].TextBoxName))
					TB->SetRenderOpacity(NewRenderOpacity);
			}
			else
			{
				if (bFadeImage)
				{
					if (UImage* Img = GetImage(EndingLogic[ActiveSequenceIndex].ImageName))
						Img->SetVisibility(ESlateVisibility::Collapsed);
				}

				for (int32 i = 0; i < EndingLogic[ActiveSequenceIndex].Texts.Num(); ++i)
					if (URichTextBlock* TB = GetTextBlock(ActiveSequenceIndex, i))
						TB->SetVisibility(ESlateVisibility::Hidden);

				ActiveSequenceIndex += 1;
				ActiveSequenceSubIndex = 0;
				HideUnusedTextBlocks(ActiveSequenceIndex);

				for (int32 i = 0; i < EndingLogic[ActiveSequenceIndex].Texts.Num(); ++i)
					if (URichTextBlock* TextBlock = GetTextBlock(ActiveSequenceIndex, i))
					{
						TextBlock->SetText(EndingLogic[ActiveSequenceIndex].Texts[i]);
						SetParentOffset(TextBlock, EndingLogic[ActiveSequenceIndex].TextOffset);
						TextBlock->SetVisibility(ESlateVisibility::Visible);
						TextBlock->SetRenderOpacity(0.0f);
					}

				bFadeImage = EndingLogic[ActiveSequenceIndex].ImageName != EndingLogic[ActiveSequenceIndex - 1].ImageName;
				EndingUIState = ESoEndingUIState::EntryFadeOutPostWait;
				FadeCounter = bFadeImage ? WaitAfterFadeOutDuration : WaitAfterTextFadeOutDuration;
			}
		}
		break;

		case ESoEndingUIState::EntryFadeOutPostWait:
			FadeCounter -= DeltaSeconds;
			if (FadeCounter < 0.0f)
			{
				if (bFadeImage)
				{
					if (UImage* Image = GetImage(EndingLogic[ActiveSequenceIndex].ImageName))
					{
						Image->SetVisibility(ESlateVisibility::Visible);
						// SetImageRenderOpacity(Image, 0.0f);
						SetImageMaterialParams(Image, EndingLogic[ActiveSequenceIndex].ImageName, true);
					}
					EndingUIState = ESoEndingUIState::EntryFadeIn;
					FadeCounter = 0.0f;
				}
				else
				{
					EndingUIState = ESoEndingUIState::EntryUpdate;
					FadeCounter = FadeInDurationFirstText;
					PlayVO(ActiveSequenceIndex, 0);
				}
			}
			break;


		case ESoEndingUIState::EntryFadeIn:
		{
			FadeCounter += DeltaSeconds;

			const float NewRenderOpacity = USoMathHelper::Interpolate(0.0f,
																	  1.0f,
																	  FMath::Clamp(FadeCounter / FadeInDurationImage, 0.0f, 1.0f),
																	  bFadeImage ? FadeInMethodImage : FadeInMethod);
			SetImageRenderOpacity(ActiveSequenceIndex, NewRenderOpacity);
			if (FadeCounter > FadeInDurationImage)
			{
				EndingUIState = ESoEndingUIState::EntryUpdateWait;
				FadeCounter = WaitAfterImgFadeDuration;
			}
		}
		break;

		case ESoEndingUIState::EntryUpdateWait:
		{
			FadeCounter -= DeltaSeconds;
			if (FadeCounter < 0.0f)
			{
				EndingUIState = ESoEndingUIState::EntryUpdate;
				FadeCounter = ActiveSequenceSubIndex == 0 ? FadeInDurationFirstText : FadeInDurationRestText;
				if (URichTextBlock* TextBlock = GetTextBlock(ConstructTextBlockName(EndingLogic[ActiveSequenceIndex].TextBoxName, ActiveSequenceSubIndex)))
				{
					if (EndingLogic[ActiveSequenceIndex].Texts.IsValidIndex(ActiveSequenceSubIndex))
					{
						TextBlock->SetText(EndingLogic[ActiveSequenceIndex].Texts[ActiveSequenceSubIndex]);
						SetParentOffset(TextBlock, EndingLogic[ActiveSequenceIndex].TextOffset);
					}

					TextBlock->SetVisibility(ESlateVisibility::Visible);
					TextBlock->SetRenderOpacity(0.0f);

					PlayVO(ActiveSequenceIndex, ActiveSequenceSubIndex);
				}
			}
		}
		break;

		case ESoEndingUIState::EntryUpdate:
		{
			FadeCounter -= DeltaSeconds;
			const float FadeDuration = ActiveSequenceSubIndex == 0 ? FadeInDurationFirstText : FadeInDurationRestText;
			const float NewRenderOpacity = USoMathHelper::Interpolate(0.0f, 1.0f, FMath::Clamp(1.0f - (FadeCounter / FadeDuration), 0.0f, 1.0f), FadeInMethod);
			SetTextBoxRenderOpacity(ActiveSequenceIndex, ActiveSequenceSubIndex, NewRenderOpacity);
			if (FadeCounter < 0.0f && !IsWaitingForVO())
			{
				if (ActiveSequenceSubIndex == 0 || ActiveSequenceSubIndex == EndingLogic[ActiveSequenceIndex].Texts.Num() - 1)
				{
					EndingUIState = ESoEndingUIState::Idle;
					ActiveSequenceSubIndex += 1;
				}
				else
				{
					EndingUIState = ESoEndingUIState::EntryUpdateWait;
					ActiveSequenceSubIndex += 1;
					FadeCounter = WaitBetweenTexts;
				}
			}
		}
		break;

		default:
			break;
	}

	return bShouldBeOpen;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UImage* USoUIEnding::GetImage(FName Name) const
{
	if (Name == NAME_None || RootCanvas == nullptr)
		return nullptr;

	return GetWidgetRecursive<UImage>(RootCanvas, Name);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
URichTextBlock* USoUIEnding::GetTextBlock(FName Name) const
{
	if (Name == NAME_None || RootCanvas == nullptr)
		return nullptr;

	return GetWidgetRecursive<URichTextBlock>(RootCanvas, Name);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
URichTextBlock* USoUIEnding::GetTextBlock(int32 SequenceIndex, int32 SubIndex) const
{
	if (EndingLogic.IsValidIndex(SequenceIndex))
		return GetTextBlock(ConstructTextBlockName(EndingLogic[SequenceIndex].TextBoxName, SubIndex));

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIEnding::RefreshPreview(float Opacity)
{
	HideAllTextBlocks();

	// Set Visibility
	for (int32 i = 0; i < EndingLogic.Num(); ++i)
	{
		if (UImage* Img = GetImage(EndingLogic[i].ImageName))
			Img->SetVisibility(ESlateVisibility::Collapsed);

		for (int32 j = 0; j < EndingLogic[i].Texts.Num(); ++j)
			if (URichTextBlock* TextBlock = GetTextBlock(ConstructTextBlockName(EndingLogic[i].TextBoxName, j)))
					TextBlock->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (EndingLogic.IsValidIndex(ActiveSequenceIndex))
	{
		if (UImage* Img = GetImage(EndingLogic[ActiveSequenceIndex].ImageName))
		{
			Img->SetVisibility(ESlateVisibility::Visible);
			SetImageRenderOpacity(Img, Opacity);
		}

		for (int32 j = 0; j < EndingLogic[ActiveSequenceIndex].Texts.Num(); ++j)
		{
			if (URichTextBlock* TextBlock = GetTextBlock(ConstructTextBlockName(EndingLogic[ActiveSequenceIndex].TextBoxName, j)))
			{
				TextBlock->SetVisibility(ESlateVisibility::Visible);
				TextBlock->SetText(EndingLogic[ActiveSequenceIndex].Texts[j]);
				SetParentOffset(TextBlock, EndingLogic[ActiveSequenceIndex].TextOffset);
				TextBlock->SetRenderOpacity(Opacity);
			}
		}
	}

	HideUnusedTextBlocks(ActiveSequenceIndex);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FName USoUIEnding::ConstructTextBlockName(FName Name, int32 Index) const
{
	return FName(*(Name.ToString() + FString::FromInt(Index)));
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIEnding::SetImageRenderOpacity(int32 SequenceIndex, float Opacity)
{
	if (UImage* Img = GetImage(EndingLogic[ActiveSequenceIndex].ImageName))
		SetImageRenderOpacity(Img, Opacity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIEnding::SetImageRenderOpacity(UImage* Image, float Opacity)
{
	if (Image == nullptr)
		return;

	if (UMaterialInstanceDynamic* DynamicMaterial = Image->GetDynamicMaterial())
		DynamicMaterial->SetScalarParameterValue(FName("FadeStr"), Opacity);
	else
		Image->SetRenderOpacity(Opacity);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIEnding::SetImageMaterialParams(UImage* Image, FName ImageName, bool bFadeIn)
{
	if (Image == nullptr)
		return;

	if (bFadeIn)
	{
		if (const FSoUIImageFadeData* FadeData = ImageFadeData.Find(ImageName))
		{
			SetImageMaterial(Image, FadeData->FadeInMaterial, ImageName, 0.0f);

			if (UMaterialInstanceDynamic* DynamicMaterial = Image->GetDynamicMaterial())
				DynamicMaterial->SetScalarParameterValue(FName("Rotation"), FadeData->FadeInRotation);
		}
		else
			SetImageMaterial(Image, ImgMaterial, ImageName, 0.0f);
	}
	else
	{
		SetImageMaterial(Image, FadeOutMaterial, ImageName, 0.0f);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIEnding::SetImageMaterial(UImage* Image, UMaterialInterface* Material, FName ImageName, float Opacity)
{
	Image->SetBrushFromMaterial(Material);
	if (UMaterialInstanceDynamic* DynamicMaterial = Image->GetDynamicMaterial())
	{
		DynamicMaterial->SetTextureParameterValue(FName("Texture"), TextureMap[ImageName]);
		DynamicMaterial->SetScalarParameterValue(FName("FadeStr"), Opacity);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIEnding::SetTextBoxRenderOpacity(int32 SequenceIndex, int32 SubIndex, float Opacity)
{
	if (URichTextBlock* TextBlock = GetTextBlock(ConstructTextBlockName(EndingLogic[SequenceIndex].TextBoxName, SubIndex)))
		TextBlock->SetRenderOpacity(Opacity);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIEnding::SetParentOffset(UWidget* Widget, const FVector2D& Offset)
{
	if (Widget != nullptr && Widget->GetParent() != nullptr)
		Widget->GetParent()->SetRenderTranslation(Offset);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIEnding::HideUnusedTextBlocks(int32 SequenceIndex)
{
	if (EndingLogic.IsValidIndex(SequenceIndex))
	{
		int32 i = EndingLogic[SequenceIndex].Texts.Num();
		while (URichTextBlock* TextBlock = GetTextBlock(SequenceIndex, i))
		{
			TextBlock->SetVisibility(ESlateVisibility::Collapsed);
			++i;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIEnding::HideAllTextBlocks()
{
	for (int32 SequenceIndex = 0; SequenceIndex < EndingLogic.Num(); ++SequenceIndex)
	{
		int32 i = 0;
		while (URichTextBlock* TextBlock = GetTextBlock(SequenceIndex, i))
		{
			TextBlock->SetVisibility(ESlateVisibility::Collapsed);
			++i;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIEnding::PlayVO(int32 SequenceIndex, int32 SubIndex)
{
	if (EndingLogic.IsValidIndex(SequenceIndex))
	{
		if (EndingLogic[SequenceIndex].VOs.IsValidIndex(SubIndex))
		{
			if (UFMODEvent* Event = EndingLogic[SequenceIndex].VOs[SubIndex])
			{
				USoAudioManager::PlaySound2D(this, Event, true);
				VOBlockTime = GetWorld()->GetTimeSeconds() + USoAudioManager::GetSoundLengthSeconds(Event);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIEnding::IsWaitingForVO() const
{
	return VOBlockTime > GetWorld()->GetTimeSeconds();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIEnding::OnFinished()
{
	if (Credits != nullptr)
	{
		Credits->FadeIn(false);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIEnding::OnCreditsFinished()
{
	USoGameInstance::GetInstance(this)->TeleportToMainMenu(false);
}

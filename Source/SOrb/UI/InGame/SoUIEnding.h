// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/PanelWidget.h"

#include "UI/InGame/SoUIGameActivity.h"
#include "Basic/Helpers/SoMathHelper.h"

#include "SoUIEnding.generated.h"

class UFMODEvent;
class UImage;
class URichTextBlock;
class UCanvasPanel;
class USoUICredits;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType, Blueprintable)
struct FSoUIEndingEntry
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ImageName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName TextBoxName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D TextOffset = FVector2D(0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Meta = (MultiLine = true))
	TArray<FText> Texts;

	/** valid VOs should match the Texts index, nullptr is used for padding */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Meta = (MultiLine = true))
	TArray<UFMODEvent*> VOs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UFMODEvent* Music;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType, Blueprintable)
struct FSoUIImageFadeData
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UMaterialInstance* FadeInMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float FadeInRotation = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float FadeOutRotation = 0.0f;
};


UENUM()
enum class ESoEndingUIState : uint8
{
	Idle = 0				UMETA(DisplayName = "Idle"),

	EntryFadeIn				UMETA(DisplayName = "EntryFadeIn"),
	EntryFadeOut			UMETA(DisplayName = "EntryFadeOut"),
	EntryFadeOutPostWait	UMETA(DisplayName = "EntryFadeOutPostWait"),

	EntryUpdate				UMETA(DisplayName = "EntryUpdate"),
	EntryUpdateWait			UMETA(DisplayName = "EntryUpdateWait"),

	FadeIn					UMETA(DisplayName = "FadeIn"),
	Finished				UMETA(DisplayName = "Finished")
};


UCLASS()
class SORB_API USoUIEnding : public USoInGameUIActivity
{
	GENERATED_BODY()

protected:
#if WITH_EDITOR
	void PreSave(const ITargetPlatform* TargetPlatform) override;
#endif // WITH_EDITOR

	// Begin UUserWidget Interface
	void NativeConstruct() override;
	void NativeDestruct() override;
	// End UUserWidget Interface

	bool SetInGameActivityEnabled_Implementation(UObject* Source, bool bEnable) override;
	bool HandleCommand_Implementation(ESoUICommand Command) override;
	bool Update_Implementation(float DeltaSeconds) override;

protected:

	UFUNCTION(BlueprintImplementableEvent)
	void StartFadeInBP();

	void OnFinished();

	UFUNCTION()
	void OnCreditsFinished();


	UFUNCTION(BlueprintPure)
	UImage* GetImage(FName Name) const;

	UFUNCTION(BlueprintPure)
	URichTextBlock* GetTextBlock(FName Name) const;

	URichTextBlock* GetTextBlock(int32 SequenceIndex, int32 SubIndex) const;

	UFUNCTION(BlueprintCallable)
	void RefreshPreview(float Opacity = 1.0f);


	FName ConstructTextBlockName(FName Name, int32 Index) const;

	template <typename WidgetType>
	WidgetType* GetWidgetRecursive(UPanelWidget* Parent, FName Name) const;


	void SetImageRenderOpacity(int32 SequenceIndex, float Opacity);
	void SetImageRenderOpacity(UImage* Image, float Opacity);
	void SetTextBoxRenderOpacity(int32 SequenceIndex, int32 SubIndex, float Opacity);

	void SetImageMaterialParams(UImage* Image, FName ImageName, bool bFadeIn);
	void SetImageMaterial(UImage* Image, UMaterialInterface* Material, FName ImageName, float Opacity);

	void SetParentOffset(UWidget* Widget, const FVector2D& Offset);

	void HideUnusedTextBlocks(int32 SequenceIndex);
	void HideAllTextBlocks();

	void PlayVO(int32 SequenceIndex, int32 SubIndex);
	bool IsWaitingForVO() const;

	virtual void UpdateLocalizedFields();

protected:

	float GetFadeOutDuration() const { return bFadeImage ? FadeOutDuration : FadeOutDurationNoImage; }

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UCanvasPanel* RootCanvas;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICredits* Credits;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Ending")
	UMaterial* ImgMaterial;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Ending")
	UMaterial* FadeOutMaterial;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ">Ending")
	int32 ActiveSequenceIndex = 0;

	int32 ActiveSequenceSubIndex = 0;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Ending")
	TArray<FSoUIEndingEntry> EndingLogic;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Ending")
	TMap<FName, FSoUIImageFadeData> ImageFadeData;

	TMap<FName, UTexture*> TextureMap;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Ending")
	float FadeOutDuration = 1.5f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Ending")
	float FadeOutDurationNoImage = 0.4f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Ending")
	ESoInterpolationMethod FadeOutMethod = ESoInterpolationMethod::EIM_SmoothStep;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Ending")
	ESoInterpolationMethod FadeOutMethodImage = ESoInterpolationMethod::EIM_Acceleration;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Ending")
	float FadeInDurationImage = 1.5f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Ending")
	float FadeInDurationFirstText = 0.5f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Ending")
	float FadeInDurationRestText = 0.6f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Ending")
	float WaitAfterImgFadeDuration = 0.3f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Ending")
	float WaitBetweenTexts = 0.6f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Ending")
	float WaitAfterFadeOutDuration = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Ending")
	float WaitAfterTextFadeOutDuration = 0.2f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Ending")
	ESoInterpolationMethod FadeInMethod = ESoInterpolationMethod::EIM_SmoothStep;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Ending")
	ESoInterpolationMethod FadeInMethodImage = ESoInterpolationMethod::EIM_Deceleration;

	float VOBlockTime = -1.0f;

	float FadeCounter;


	ESoEndingUIState EndingUIState = ESoEndingUIState::Idle;

	UPROPERTY(BlueprintReadWrite, Category = ">Ending")
	bool bShouldBeOpen = false;

	bool bFadeImage = false;
};


template <typename WidgetType>
WidgetType* USoUIEnding::GetWidgetRecursive(UPanelWidget* Parent, FName Name) const
{
	if (Name == NAME_None || Parent == nullptr)
		return nullptr;

	for (int32 i = 0; i < Parent->GetChildrenCount(); ++i)
	{
		if (WidgetType* ChildWidget = Cast<WidgetType>(Parent->GetChildAt(i)))
		{
			if (ChildWidget->GetFName() == Name)
				return ChildWidget;
		}
		else if (UPanelWidget* Panel = Cast<UPanelWidget>(Parent->GetChildAt(i)))
		{
			if (WidgetType* RecursiveWidget = GetWidgetRecursive<WidgetType>(Panel, Name))
				return RecursiveWidget;
		}
	}

	return nullptr;
}

// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once


#include "CoreMinimal.h"
#include "UI/General/SoUIConfirmPanel.h"

#include "SoUIConfirmQuestion.generated.h"


class UTextBlock;

UENUM(BlueprintType)
enum class ESoUIConfirmQuestionAnswerType : uint8
{
	// Invalid
	None = 0,

	Yes,
	No,
};

UCLASS()
class SORB_API USoUIConfirmQuestion : public USoUIConfirmPanel
{
	GENERATED_BODY()

public:
	//
	// UUserWidget Interface
	//
	void SynchronizeProperties() override;
	void NativeConstruct() override;

	//
	// Own methods
	//

	bool IsAnswerYes() const { return ConfirmAnswerType == ESoUIConfirmQuestionAnswerType::Yes; }
	bool IsAnswerNo() const { return ConfirmAnswerType == ESoUIConfirmQuestionAnswerType::No; }
	bool IsValidAnswer() const { return ConfirmAnswerType != ESoUIConfirmQuestionAnswerType::None; }

	// If the SubText is empty then it will not be displayed
	void SetTexts(FText Text, FText SubText);

	void SetPlayBackToMenuRootSFXOnNo(bool bPlay) { bPlayBackToMenuRootSFXOnNo = bPlay; }

protected:
	//
	// USoUIEventHandler Interface
	//
	void Open_Implementation(bool bOpen) override;
	bool OnUICommand_Implementation(ESoUICommand Command) override;

	//
	// USoUIConfirmPanel interface
	//

	// Sets/Creates the texts and children for all lines
	// NOTE: does not depend on any runtime value
	void RefreshButtonsArray() override;


	//
	// Own methods
	//

	void UpdateFromCurrentAnswer();
	void SetAnswer(ESoUIConfirmQuestionAnswerType NewAnswer);
	void ResetAnswer()
	{
		ConfirmAnswerType = ESoUIConfirmQuestionAnswerType::None;
	}

	int32 GetAnswerButtonOptionIndexFor(ESoUIConfirmQuestionAnswerType AnswerType) const
	{
		return AnswerButtonOption.Find(AnswerType);
	}

protected:
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* TextTitle = nullptr;

	// Text that appears underneath the TextTitle
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* TextSubTitle = nullptr;

	// Tells us if the user responded to the confirm
	// If None it means the user did not answer
	UPROPERTY(BlueprintReadOnly, Category = ">Options")
	ESoUIConfirmQuestionAnswerType ConfirmAnswerType = ESoUIConfirmQuestionAnswerType::None;

	// Map selected button index => ESoUIConfirmQuitAnswerType (button type)
	const TArray<ESoUIConfirmQuestionAnswerType> AnswerButtonOption = {
		ESoUIConfirmQuestionAnswerType::Yes,
		ESoUIConfirmQuestionAnswerType::No
	};

	// The default selected answer when
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">Options")
	ESoUIConfirmQuestionAnswerType DefaultSelectedAnswerType = ESoUIConfirmQuestionAnswerType::No;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	bool bPlayBackToMenuRootSFXOnNo = true;
};

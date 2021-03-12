// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"
#include "SoDialogueParticipant.generated.h"

class UTexture2D;
class UFMODEvent;

/** Struct containing all the UI relevant dialogue parameters */
USTRUCT(BlueprintType)
struct FSoDialogueParticipantData
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DialogueUIData)
	FName ParticipantName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DialogueUIData)
	FText ParticipantDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DialogueUIData)
	int32 PositionIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DialogueUIData)
	FName FadeInAnimName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DialogueUIData)
	FName FadeOutAnimName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DialogueUIData)
	float TextBoxY = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DialogueUIData)
	FVector2D FloorPosition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DialogueUIData)
	UTexture2D* ParticipantIcon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DialogueUIData)
	UTexture2D* FloorImage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DialogueUIData)
	UTexture2D* NameTextBackground = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DialogueUIData)
	UTexture2D* TextBoxBackground = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DialogueUIData)
	UFMODEvent* Music = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DialogueUIData)
	float PrevMusicFadeDuration = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DialogueUIData)
	bool bKeepMusicAfterDialogue = false;

public:

	static FSoDialogueParticipantData InvalidValue;
};


/**
 *
 */
UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class SORB_API USoDialogueParticipant : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

/**
 *   C++ only interface to get dialogue data
 *   All Dialogue participants must implement it
 *   To call the function simply use Cast<ISoDialogueParticipant>(Object)->GetParticipantData(), no Execute_ function is generated
 */
class SORB_API ISoDialogueParticipant
{
	GENERATED_IINTERFACE_BODY()

	UFUNCTION(BlueprintCallable, Category = WaitWhat)
	virtual const FSoDialogueParticipantData& GetParticipantData() const { return FSoDialogueParticipantData::InvalidValue; }
};

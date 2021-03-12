// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "UObject/Interface.h"

#include "Settings/Input/SoInputNames.h"

#include "SoInteractable.generated.h"

class ASoCharacter;


USTRUCT(BlueprintType)
struct FSoInteractableMessage
{
	GENERATED_USTRUCT_BODY()

public:
	// The input action name UI, optional, only displayed if this is valid
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESoInputActionNameType ActionNameType = ESoInputActionNameType::IANT_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayText;
};


UINTERFACE()
class SORB_API USoInteractable : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};


class SORB_API ISoInteractable
{
	GENERATED_IINTERFACE_BODY()

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Interaction)
	void Interact(ASoCharacter* Character);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Interaction)
	FText GetInteractionText() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Interaction)
	FSoInteractableMessage GetInteractionMessage() const;

	/** All other interactable is ignored until the player is in the area of this one. The first exclusive is the active one */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Interaction)
	bool IsExclusive() const;

	/** Weather first or second key is used (if it is still free) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Interaction)
	bool IsSecondKeyPrefered() const;


	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Interaction)
	bool CanBeInteractedWithFromAir() const;
};

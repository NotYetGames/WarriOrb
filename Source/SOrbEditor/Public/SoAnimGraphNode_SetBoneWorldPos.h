// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "AnimGraphNode_SkeletalControlBase.h"

// game:
#include "Anim/SoAnimNode_SetBoneWorldPos.h"

#include "SoAnimGraphNode_SetBoneWorldPos.generated.h"


/**
 *
 */
UCLASS()
class SORBEDITOR_API USoAnimGraphNode_SetBoneWorldPos : public UAnimGraphNode_SkeletalControlBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Settings)
	FSoAnimNode_SetBoneWorldPos Node;

public:
	// UEdGraphNode interface
	virtual FText GetTooltipText() const override;
	// End of UEdGraphNode interface

protected:
	// Returns the short descriptive name of the controller
	virtual FText GetControllerDescription() const override;
	virtual const FSoAnimNode_SetBoneWorldPos* GetNode() const override { return &Node; }

};

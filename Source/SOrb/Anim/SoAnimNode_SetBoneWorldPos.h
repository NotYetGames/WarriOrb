// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "SoAnimNode_SetBoneWorldPos.generated.h"

/**
 *
 */
USTRUCT()
struct SORB_API FSoAnimNode_SetBoneWorldPos : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

	/** Name of bone to control. **/
	UPROPERTY(EditAnywhere)
	FBoneReference IKBone;

	/** Target Location to reach. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (PinShownByDefault))
	FVector BoneLocation{EForceInit::ForceInitToZero};

	FSoAnimNode_SetBoneWorldPos();

	// FAnimNode_Base interface
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface

	// FAnimNode_SkeletalControlBase interface
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

private:
	// FAnimNode_SkeletalControlBase interface
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface
};

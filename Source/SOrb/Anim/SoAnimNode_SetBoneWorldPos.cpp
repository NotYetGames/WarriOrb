// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoAnimNode_SetBoneWorldPos.h"
// #include "Animation/AnimTypes.h"
 #include "AnimationRuntime.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimInstanceProxy.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoAnimNode_SetBoneWorldPos::FSoAnimNode_SetBoneWorldPos()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoAnimNode_SetBoneWorldPos::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);

	DebugLine += "(";
	AddDebugNodeData(DebugLine);
	DebugLine += FString::Printf(TEXT(" IKBone: %s)"), *IKBone.BoneName.ToString());
	DebugData.AddDebugItem(DebugLine);

	ComponentPose.GatherDebugData(DebugData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoAnimNode_SetBoneWorldPos::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	// SCOPE_CYCLE_COUNTER(STAT_TwoBoneIK_Eval);

	check(OutBoneTransforms.Num() == 0);

	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();

	// Get indices of the lower and upper limb bones and check validity.
	bool bInvalidIndex = false;

	FCompactPoseBoneIndex IKBoneCompactPoseIndex = IKBone.GetCompactPoseIndex(BoneContainer);


	// Get Local Space transforms for our bones. We do this first in case they already are local.
	// As right after we get them in component space. (And that does the auto conversion).
	// We might save one transform by doing local first...
	const FTransform EndBoneLocalTransform = Output.Pose.GetLocalSpaceTransform(IKBoneCompactPoseIndex);
	// Now get those in component space...
	FTransform EndBoneCSTransform = Output.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex);

	// Transform EffectorLocation from EffectorLocationSpace to ComponentSpace.
	FTransform TargetTransform(BoneLocation);
	FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
	FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, TargetTransform, FCompactPoseBoneIndex{ INDEX_NONE }, EBoneControlSpace::BCS_WorldSpace);

	// This is our reach goal.
	FVector DesiredPos = TargetTransform.GetTranslation();

	EndBoneCSTransform.SetTranslation(DesiredPos);
	OutBoneTransforms.Add(FBoneTransform(IKBoneCompactPoseIndex, EndBoneCSTransform));

	// Make sure we have correct number of bones
	check(OutBoneTransforms.Num() == 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoAnimNode_SetBoneWorldPos::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	// if both bones are valid
	return IKBone.IsValidToEvaluate(RequiredBones);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoAnimNode_SetBoneWorldPos::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	IKBone.Initialize(RequiredBones);
}

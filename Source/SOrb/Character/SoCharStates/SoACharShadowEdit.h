// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "SoActivity.h"
#include "SplineLogic/SoCharShadowData.h"
#include "SoACharShadowEdit.generated.h"


UCLASS()
class SORB_API USoACharShadowEdit : public USoActivity
{

	GENERATED_BODY()

public:

	USoACharShadowEdit();

	virtual void Tick(float DeltaSeconds) override;	
	virtual void UpdateCharMaterials(float DeltaSeconds) override;

	// show/hide help text
	virtual void InteractKeyPressed(bool bPrimaryKey) override;
	
	// save/load from file
	void SaveEditedData() override;
	void LoadEditedData() override;
	// add/move/remove key node
	void CreateKey() override;
	void MoveClosestKeyHere() override;
	void DeleteActiveKeyNode() override;

	virtual void CopyActiveKeyData() override;
	virtual void PasteToActiveKeyData() override;

	// teleport the character into the KeyNode position
	void JumpToNextKey() override;
	void JumpToPrevKey() override;

	virtual void ToggleWeapons() override {};

	virtual void SuperEditModePressed() override;

	// get data for gui
	UFUNCTION(BlueprintCallable, Category = UIInfo)
	bool IsOnKey() const { return bOnKeyPosition; }

	UFUNCTION(BlueprintCallable, Category = UIInfo)
	FSoCharShadowKeyNode GetInterpolatedKey() const;

	UFUNCTION(BlueprintCallable, Category = UIInfo)
	const FSoCharShadowKeyNode& GetPrevKeyNodeData() const;

	UFUNCTION(BlueprintCallable, Category = UIInfo)
	const FSoCharShadowKeyNode& GetNextKeyNodeData() const;

	UFUNCTION(BlueprintCallable, Category = UIInfo)
	int32 GetNodeCount() const;

protected:

	virtual void OnEnter(USoActivity* OldActivity) override;
	virtual void OnExit(USoActivity* NewActivity) override;

protected:

	UPROPERTY(BlueprintReadOnly, Category = Sky)
	bool bOnKeyPosition = false;

	UPROPERTY(BlueprintReadOnly, Category = Sky)
	bool bEditHintVisible = false;

	// protection from everything + the ability to fly
	bool bSuperMode = false;

	FSoCharShadowKeyNode NodeCopy;
	bool bNodeCopyInitialized = false;
};


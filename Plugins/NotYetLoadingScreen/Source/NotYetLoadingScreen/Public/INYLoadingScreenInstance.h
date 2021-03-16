// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"

class UTexture2D;

/**
 * The public interface to this module
 */
class NOTYETLOADINGSCREEN_API INYLoadingScreenInstance
{
public:
	virtual ~INYLoadingScreenInstance() {}
	
	virtual void SetAutoShowLoadingScreen(bool bInValue) = 0;
	virtual void SetWaitForAnyKeyInput(bool bInValue, bool bFocus = true) = 0;
	virtual bool IsWaitForAnyKeyInput() const = 0;
	virtual void SetWaitForAnyKeyInputData(TSoftObjectPtr<UTexture2D> Image, FText Title, FText Description) = 0;

	virtual void UpdateLoadingScreenTip(FText Tip) = 0;

	// This can fail is you set SetWaitForAnyKeyInput(true) but no input was received
	virtual bool CanHideLoadingScreen() const = 0;

	virtual bool ShowLoadingScreen(bool bPlayUntilStopped = true, float PlayTime = -1.f) = 0;
	virtual bool HideLoadingScreen() = 0;
	virtual bool IsLoadingScreenVisible() const = 0;
	virtual void RedrawLoadingScreenWidget() = 0;

	virtual void TestShowWidget() = 0;
	virtual void TestHideWidget() = 0;
};

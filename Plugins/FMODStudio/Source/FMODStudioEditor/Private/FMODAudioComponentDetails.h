// Copyright (c), Firelight Technologies Pty, Ltd. 2012-2020.

#pragma once

//#include "PropertyEditing.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditor/Public/IDetailCustomization.h"

class FFMODAudioComponentDetails : public IDetailCustomization
{
public:
    /** Makes a new instance of this detail layout class for a specific detail view requesting it */
    static TSharedRef<IDetailCustomization> MakeInstance();

private:
    /** IDetailCustomization interface */
    virtual void CustomizeDetails(IDetailLayoutBuilder &DetailBuilder) override;

    FReply OnEditSoundClicked();
    FReply OnPlaySoundClicked();
    FReply OnStopSoundClicked();

    TWeakObjectPtr<class UFMODAudioComponent> AudioComponent;
};
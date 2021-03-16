// Copyright (c), Firelight Technologies Pty, Ltd. 2012-2020.

#pragma once

#include "Containers/UnrealString.h"
#include "Misc/DateTime.h"
#include "Delegates/Delegate.h"

class FFMODBankUpdateNotifier
{
public:
    FFMODBankUpdateNotifier();

    void SetFilePath(const FString &InPath);
    void Update();

    void EnableUpdate(bool bEnable);

    FSimpleMulticastDelegate BanksUpdatedEvent;

private:
    void Refresh();

    bool bUpdateEnabled;
    FString FilePath;
    FDateTime NextRefreshTime;
    FDateTime FileTime;
};

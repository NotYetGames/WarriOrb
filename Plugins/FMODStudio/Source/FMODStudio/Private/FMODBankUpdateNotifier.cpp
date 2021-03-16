// Copyright (c), Firelight Technologies Pty, Ltd. 2012-2020.

#include "FMODBankUpdateNotifier.h"
#include "FMODSettings.h"
#include "HAL/FileManager.h"

#include "FMODStudioPrivatePCH.h"

FFMODBankUpdateNotifier::FFMODBankUpdateNotifier()
    : bUpdateEnabled(true)
    , NextRefreshTime(FDateTime::MinValue())
{
}

void FFMODBankUpdateNotifier::SetFilePath(const FString &InPath)
{
    FilePath = InPath;
    NextRefreshTime = FDateTime::MinValue();
    FileTime = FDateTime::MinValue();
}

void FFMODBankUpdateNotifier::Update()
{
    if (bUpdateEnabled)
    {
        FDateTime CurTime = FDateTime::UtcNow();
        if (CurTime >= NextRefreshTime)
        {
            NextRefreshTime = CurTime + FTimespan(0, 0, 1);
            Refresh();
        }
    }
}

void FFMODBankUpdateNotifier::EnableUpdate(bool bEnable)
{
    bUpdateEnabled = bEnable;

    if (bEnable)
    {
        // Refreshing right after update is enabled is not desirable
        NextRefreshTime = FDateTime::UtcNow() + FTimespan(0, 0, 1);
    }
}

void FFMODBankUpdateNotifier::Refresh()
{
    if (!FilePath.IsEmpty())
    {
        const FDateTime NewFileTime = IFileManager::Get().GetTimeStamp(*FilePath);
        if (NewFileTime != FileTime)
        {
            FileTime = NewFileTime;
            UE_LOG(LogFMOD, Log, TEXT("File has changed: %s"), *FilePath);

            BanksUpdatedEvent.Broadcast();
        }
    }
}

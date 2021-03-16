// Copyright (c), Firelight Technologies Pty, Ltd. 2012-2020.

#pragma once

#include "FMODAsset.h"

namespace FMOD
{
namespace Studio
{
class System;
}
}

class FFMODAssetTable
{
public:
    FFMODAssetTable();
    ~FFMODAssetTable();

    void Create();
    void Destroy();

    void Refresh();

    UFMODAsset *FindByName(const FString &Name);

private:
    void HandleBanksUpdated();
    void AddAsset(const FGuid &AssetGuid, const FString &AssetFullName);

private:
    FMOD::Studio::System *StudioSystem;
    TMap<FGuid, TWeakObjectPtr<UFMODAsset>> GuidMap;
    TMap<FName, TWeakObjectPtr<UFMODAsset>> NameMap;
    TMap<FString, TWeakObjectPtr<UFMODAsset>> FullNameLookup;
};

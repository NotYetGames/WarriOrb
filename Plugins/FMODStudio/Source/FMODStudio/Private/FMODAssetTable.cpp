// Copyright (c), Firelight Technologies Pty, Ltd. 2012-2020.

#include "FMODAssetTable.h"
#include "FMODEvent.h"
#include "FMODSnapshot.h"
#include "FMODSnapshotReverb.h"
#include "FMODBank.h"
#include "FMODBus.h"
#include "FMODVCA.h"
#include "FMODUtils.h"
#include "FMODSettings.h"
#include "FMODFileCallbacks.h"
#include "FMODStudioPrivatePCH.h"
#include "fmod_studio.hpp"
#include "UObject/Package.h"

#if WITH_EDITOR
#include "AssetRegistryModule.h"
#endif

FFMODAssetTable::FFMODAssetTable()
    : StudioSystem(nullptr)
{
}

FFMODAssetTable::~FFMODAssetTable()
{
    Destroy();
}

void FFMODAssetTable::Create()
{
    Destroy();

    // Create a sandbox system purely for loading and considering banks
    verifyfmod(FMOD::Studio::System::create(&StudioSystem));
    FMOD::System *lowLevelSystem = nullptr;
    verifyfmod(StudioSystem->getLowLevelSystem(&lowLevelSystem));
    verifyfmod(lowLevelSystem->setOutput(FMOD_OUTPUTTYPE_NOSOUND));
    AttachFMODFileSystem(lowLevelSystem, 2048);
    verifyfmod(
        StudioSystem->initialize(1, FMOD_STUDIO_INIT_ALLOW_MISSING_PLUGINS | FMOD_STUDIO_INIT_SYNCHRONOUS_UPDATE, FMOD_INIT_MIX_FROM_UPDATE, 0));
}

void FFMODAssetTable::Destroy()
{
    if (StudioSystem != nullptr)
    {
        verifyfmod(StudioSystem->release());
    }
    StudioSystem = nullptr;
}

UFMODAsset *FFMODAssetTable::FindByName(const FString &Name)
{
    TWeakObjectPtr<UFMODAsset> *FoundAsset = FullNameLookup.Find(Name);
    if (FoundAsset)
    {
        return FoundAsset->Get();
    }
    return nullptr;
}

void FFMODAssetTable::Refresh()
{
    if (StudioSystem == nullptr)
    {
        return;
    }

    const UFMODSettings &Settings = *GetDefault<UFMODSettings>();
    FString StringPath = Settings.GetMasterStringsBankPath();

    UE_LOG(LogFMOD, Log, TEXT("Loading strings bank: %s"), *StringPath);

    FMOD::Studio::Bank *StudioStringBank;
    FMOD_RESULT StringResult = StudioSystem->loadBankFile(TCHAR_TO_UTF8(*StringPath), FMOD_STUDIO_LOAD_BANK_NORMAL, &StudioStringBank);
    if (StringResult == FMOD_OK)
    {
        TArray<char> RawBuffer;
        RawBuffer.SetNum(256); // Initial capacity

        int Count = 0;
        verifyfmod(StudioStringBank->getStringCount(&Count));
        for (int StringIdx = 0; StringIdx < Count; ++StringIdx)
        {
            FMOD_RESULT Result;
            FMOD::Studio::ID Guid = { 0 };
            while (true)
            {
                int ActualSize = 0;
                Result = StudioStringBank->getStringInfo(StringIdx, &Guid, RawBuffer.GetData(), RawBuffer.Num(), &ActualSize);
                if (Result == FMOD_ERR_TRUNCATED)
                {
                    RawBuffer.SetNum(ActualSize);
                }
                else
                {
                    break;
                }
            }
            verifyfmod(Result);
            FString AssetName(UTF8_TO_TCHAR(RawBuffer.GetData()));
            FGuid AssetGuid = FMODUtils::ConvertGuid(Guid);
            if (!AssetName.IsEmpty())
            {
                AddAsset(AssetGuid, AssetName);
            }
        }
        verifyfmod(StudioStringBank->unload());
        verifyfmod(StudioSystem->update());
    }
    else
    {
        UE_LOG(LogFMOD, Warning, TEXT("Failed to load strings bank: %s"), *StringPath);
    }
}

void FFMODAssetTable::AddAsset(const FGuid &AssetGuid, const FString &AssetFullName)
{
    FString AssetPath = AssetFullName;
    FString AssetType = "";
    FString AssetFileName = "asset";

    int DelimIndex;
    if (AssetPath.FindChar(':', DelimIndex))
    {
        AssetType = AssetPath.Left(DelimIndex);
        AssetPath = AssetPath.Right(AssetPath.Len() - DelimIndex - 1);
    }

    FString FormattedAssetType = "";
    UClass *AssetClass = UFMODAsset::StaticClass();
    if (AssetType.Equals(TEXT("event")))
    {
        FormattedAssetType = TEXT("Events");
        AssetClass = UFMODEvent::StaticClass();
    }
    else if (AssetType.Equals(TEXT("snapshot")))
    {
        FormattedAssetType = TEXT("Snapshots");
        AssetClass = UFMODSnapshot::StaticClass();
    }
    else if (AssetType.Equals(TEXT("bank")))
    {
        FormattedAssetType = TEXT("Banks");
        AssetClass = UFMODBank::StaticClass();
    }
    else if (AssetType.Equals(TEXT("bus")))
    {
        FormattedAssetType = TEXT("Buses");
        AssetClass = UFMODBus::StaticClass();
    }
    else if (AssetType.Equals(TEXT("vca")))
    {
        FormattedAssetType = TEXT("VCAs");
        AssetClass = UFMODVCA::StaticClass();
    }
    else
    {
        UE_LOG(LogFMOD, Warning, TEXT("Unknown asset type: %s"), *AssetType);
    }

    if (AssetPath.FindLastChar('/', DelimIndex))
    {
        AssetFileName = AssetPath.Right(AssetPath.Len() - DelimIndex - 1);
        AssetPath = AssetPath.Left(AssetPath.Len() - AssetFileName.Len() - 1);
    }
    else
    {
        // No path part, all name
        AssetFileName = AssetPath;
        AssetPath = TEXT("");
    }

    if (AssetFileName.IsEmpty() || AssetFileName.Contains(TEXT(".strings")))
    {
        UE_LOG(LogFMOD, Log, TEXT("Skipping asset: %s"), *AssetFullName);
        return;
    }

    AssetPath = AssetPath.Replace(TEXT(" "), TEXT("_"));
    FString AssetShortName = AssetFileName.Replace(TEXT(" "), TEXT("_"));
    AssetShortName = AssetShortName.Replace(TEXT("."), TEXT("_"));

    const UFMODSettings &Settings = *GetDefault<UFMODSettings>();

    FString FolderPath = Settings.ContentBrowserPrefix;
    FolderPath += FormattedAssetType;
    FolderPath += AssetPath;

    FString AssetPackagePath = FolderPath + TEXT("/") + AssetShortName;

    FName AssetPackagePathName(*AssetPackagePath);

    TWeakObjectPtr<UFMODAsset> &ExistingNameAsset = NameMap.FindOrAdd(AssetPackagePathName);
    TWeakObjectPtr<UFMODAsset> &ExistingGuidAsset = GuidMap.FindOrAdd(AssetGuid);
    TWeakObjectPtr<UFMODAsset> &ExistingFullNameLookupAsset = FullNameLookup.FindOrAdd(AssetFullName);

    UFMODAsset *AssetNameObject = ExistingNameAsset.Get();
    if (AssetNameObject == nullptr)
    {
        UE_LOG(LogFMOD, Log, TEXT("Constructing asset: %s"), *AssetPackagePath);

        EObjectFlags NewObjectFlags = RF_Standalone | RF_Public /* | RF_Transient */;
        if (IsRunningDedicatedServer())
        {
            NewObjectFlags |= RF_MarkAsRootSet;
        }

        UPackage *NewPackage = CreatePackage(NULL, *AssetPackagePath);
        if (IsValid(NewPackage))
        {
            if (!GEventDrivenLoaderEnabled)
            {
                NewPackage->SetPackageFlags(PKG_CompiledIn);
            }

            AssetNameObject = NewObject<UFMODAsset>(NewPackage, AssetClass, FName(*AssetShortName), NewObjectFlags);
            AssetNameObject->AssetGuid = AssetGuid;
            AssetNameObject->bShowAsAsset = true;
            AssetNameObject->FileName = AssetFileName;

#if WITH_EDITOR
            FAssetRegistryModule &AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
            AssetRegistryModule.Get().AddPath(*FolderPath);
            FAssetRegistryModule::AssetCreated(AssetNameObject);
#endif
        }
        else
        {
            UE_LOG(LogFMOD, Warning, TEXT("Failed to construct package for asset %s"), *AssetPackagePath);
        }

        if (AssetClass == UFMODSnapshot::StaticClass())
        {
            FString ReverbFolderPath = Settings.ContentBrowserPrefix;
            ReverbFolderPath += TEXT("Reverbs");
            ReverbFolderPath += AssetPath;

            FString ReverbAssetPackagePath = ReverbFolderPath + TEXT("/") + AssetShortName;

            UPackage *ReverbPackage = CreatePackage(NULL, *ReverbAssetPackagePath);
            if (ReverbPackage)
            {
                if (!GEventDrivenLoaderEnabled)
                {
                    ReverbPackage->SetPackageFlags(PKG_CompiledIn);
                }
                UFMODSnapshotReverb *AssetReverb = NewObject<UFMODSnapshotReverb>(
                    ReverbPackage, UFMODSnapshotReverb::StaticClass(), FName(*AssetShortName), NewObjectFlags);
                AssetReverb->AssetGuid = AssetGuid;
                AssetReverb->bShowAsAsset = true;

#if WITH_EDITOR
                FAssetRegistryModule &AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
                AssetRegistryModule.Get().AddPath(*ReverbFolderPath);
                FAssetRegistryModule::AssetCreated(AssetReverb);
#endif
            }
        }
    }

    UFMODAsset *AssetGuidObject = ExistingGuidAsset.Get();
    if (IsValid(AssetGuidObject) && AssetGuidObject != AssetNameObject)
    {
        FString OldPath = AssetGuidObject->GetPathName();
        UE_LOG(LogFMOD, Log, TEXT("Hiding old asset '%s'"), *OldPath);

        // We had an asset with the same guid but it must have been renamed
        // We just hide the old asset from the asset table
        AssetGuidObject->bShowAsAsset = false;

#if WITH_EDITOR
        FAssetRegistryModule::AssetRenamed(AssetNameObject, OldPath);
#endif
    }

    ExistingNameAsset = AssetNameObject;
    ExistingGuidAsset = AssetNameObject;
    ExistingFullNameLookupAsset = AssetNameObject;
}

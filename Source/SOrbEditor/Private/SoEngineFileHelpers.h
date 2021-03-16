// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "FileHelpers.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Misc/MessageDialog.h"
#include "EditorStyleSet.h"
#include "Logging/TokenizedMessage.h"
#include "CoreGlobals.h"
#include "HAL/FileManager.h"

#include "SOrbEditorModule.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "ISourceControlModule.h"
#include "Misc/FileHelper.h"
#include "EditorDirectories.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "BusyCursor.h"
#include "Misc/RedirectCollector.h"
#include "Misc/ScopedSlowTask.h"
#include "ObjectTools.h"
#include "EditorModes.h"
#include "Engine/MapBuildDataRegistry.h"
#include "PackageTools.h"
#include "EditorModeManager.h"

namespace SoEngineFileHelpers
{
#define LOCTEXT_NAMESPACE "WarriorbEditorEngineFileHelpers"


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// Copied from FileHelpers.cpp
	// Simulates FEditorFileUtils::SaveDirtyPackages
	//
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class InternalSavePackageResult : int8
	{
		Success,
		Cancel,
		Continue,
		Error,
	};

	/** A special output device that puts save output in the message log when flushed */
	class FSaveErrorOutputDevice : public FOutputDevice
	{
	public:
		virtual void Serialize(const TCHAR* InData, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			if (Verbosity == ELogVerbosity::Error || Verbosity == ELogVerbosity::Warning)
			{
				EMessageSeverity::Type Severity;
				if (Verbosity == ELogVerbosity::Error)
				{
					Severity = EMessageSeverity::Error;
				}
				else
				{
					Severity = EMessageSeverity::Warning;
				}
				ErrorMessages.Add(FTokenizedMessage::Create(Severity, FText::FromName(InData)));
			}
		}

		virtual void Flush() override
		{
			if (ErrorMessages.Num() > 0)
			{
				FMessageLog EditorErrors("EditorErrors");
				EditorErrors.NewPage(LOCTEXT("SaveOutputPageLabel", "Save Output"));
				EditorErrors.AddMessages(ErrorMessages);
				EditorErrors.Open();
				ErrorMessages.Empty();
			}
		}

	private:
		// Holds the errors for the message log.
		TArray< TSharedRef< FTokenizedMessage > > ErrorMessages;
	};

	static TArray<UPackage*> InternalGetDirtyPackages(const bool bSaveMapPackages, const bool bSaveContentPackages)
	{
		if (bSaveContentPackages)
		{
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
		}

		// A list of all packages that need to be saved
		TArray<UPackage*> PackagesToSave;

		if (bSaveMapPackages)
		{
			FEditorFileUtils::GetDirtyWorldPackages(PackagesToSave);
		}

		// Don't iterate through content packages if we don't plan on saving them
		if (bSaveContentPackages)
		{
			FEditorFileUtils::GetDirtyContentPackages(PackagesToSave);
		}

		return PackagesToSave;
	}

	static void InternalNotifyNoPackagesSaved(const bool bUseDialog)
	{
		UE_LOG(LogSoEditor, Warning, TEXT("%s"), *LOCTEXT("NoAssetsToSave", "No new changes to save!").ToString());
		if (bUseDialog)
		{
			FNotificationInfo NotificationInfo(LOCTEXT("NoAssetsToSave", "No new changes to save!"));
			NotificationInfo.Image = FEditorStyle::GetBrush(FTokenizedMessage::GetSeverityIconName(EMessageSeverity::Info));
			NotificationInfo.bFireAndForget = true;
			NotificationInfo.ExpireDuration = 4.0f; // Need this message to last a little longer than normal since the user may have expected there to be modified files.
			NotificationInfo.bUseThrobber = true;
			FSlateNotificationManager::Get().AddNotification(NotificationInfo);
		}
	}

	static void InternalWarnUserAboutFailedSave(const TArray<UPackage*>& InFailedPackages, bool bUseDialog)
	{
		// Warn the user if any packages failed to save
		if (InFailedPackages.Num() > 0)
		{
			FString FailedPackages;
			for (TArray<UPackage*>::TConstIterator FailedIter(InFailedPackages); FailedIter; ++FailedIter)
			{
				FailedPackages += FString::Printf(TEXT("\n%s"), *((*FailedIter)->GetName()));
			}

			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("Packages"), FText::FromString(FailedPackages));
			const FText MessageFormatting = NSLOCTEXT("FileHelper", "FailedSavePromptMessageFormatting", "The following assets failed to save correctly:{Packages}");
			const FText Message = FText::Format(MessageFormatting, Arguments);

			// Display warning
			UE_LOG(LogSoEditor, Error, TEXT("%s"), *Message.ToString());
			if (bUseDialog)
			{
				FText Title = NSLOCTEXT("FileHelper", "FailedSavePrompt_Title", "Packages Failed To Save");
				FMessageDialog::Open(EAppMsgType::Ok, Message, &Title);
			}
		}
	}

	static FString GetDefaultDirectory()
	{
		return FEditorDirectories::Get().GetLastDirectory(ELastDirectory::UNR);
	}

	static FString GetAutoSaveDir()
	{
		return FPaths::ProjectSavedDir() / TEXT("Autosaves");
	}


	static bool OpenSaveAsDialog(UClass* SavedClass, const FString& InDefaultPath, const FString& InNewNameSuggestion, FString& OutPackageName)
	{
		FString DefaultPath = InDefaultPath;

		if (DefaultPath.IsEmpty())
		{
			DefaultPath = TEXT("/Game/Maps");
		}

		const FString NewNameSuggestion = InNewNameSuggestion;
		check(!NewNameSuggestion.IsEmpty());

		FSaveAssetDialogConfig SaveAssetDialogConfig;
		{
			SaveAssetDialogConfig.DefaultPath = DefaultPath;
			SaveAssetDialogConfig.DefaultAssetName = NewNameSuggestion;
			SaveAssetDialogConfig.AssetClassNames.Add(SavedClass->GetFName());
			SaveAssetDialogConfig.ExistingAssetPolicy = ESaveAssetDialogExistingAssetPolicy::AllowButWarn;
			SaveAssetDialogConfig.DialogTitleOverride = (SavedClass == UWorld::StaticClass())
				? LOCTEXT("SaveLevelDialogTitle", "Save Level As")
				: LOCTEXT("SaveAssetDialogTitle", "Save Asset As");
		}

		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		const FString SaveObjectPath = ContentBrowserModule.Get().CreateModalSaveAssetDialog(SaveAssetDialogConfig);

		if (!SaveObjectPath.IsEmpty())
		{
			OutPackageName = FPackageName::ObjectPathToPackageName(SaveObjectPath);
			return true;
		}

		return false;
	}

	/**
	 * Queries the user if they want to quit out of interpolation editing before save.
	 *
	 * @return		true if in interpolation editing mode, false otherwise.
	 */
	static bool InInterpEditMode()
	{
		// Must exit Interpolation Editing mode before you can save - so it can reset everything to its initial state.
		if (GLevelEditorModeTools().IsModeActive(FBuiltinEditorModes::EM_InterpEdit))
		{
			const bool ExitInterp = EAppReturnType::Yes == FMessageDialog::Open(EAppMsgType::YesNo, EAppReturnType::Yes, NSLOCTEXT("UnrealEd", "Prompt_21", "You must close Matinee before saving level.\nDo you wish to do this now and continue?"));
			if (!ExitInterp)
			{
				return true;
			}

			GLevelEditorModeTools().DeactivateMode(FBuiltinEditorModes::EM_InterpEdit);
		}
		return false;
	}

	/**
	 * @param	World					The world to save.
	 * @param	ForceFilename			If non-NULL, save the level package to this name (full path+filename).
	 * @param	OverridePath			If non-NULL, override the level path with this path.
	 * @param	FilenamePrefix			If non-NULL, prepend this string the the level filename.
	 * @param	bRenamePackageToFile	If true, rename the level package to the filename if save was successful.
	 * @param	bCheckDirty				If true, don't save the level if it is not dirty.
	 * @param	FinalFilename			[out] The full path+filename the level was saved to.
	 * @param	bAutosaving				Should be set to true if autosaving; passed to UWorld::SaveWorld.
	 * @param	bPIESaving				Should be set to true if saving for PIE; passed to UWorld::SaveWorld.
	 * @return							true if the level was saved.
	 */
	static bool InternalSaveWorld(UWorld* World,
		const FString* ForceFilename,
		const TCHAR* OverridePath,
		const TCHAR* FilenamePrefix,
		bool bRenamePackageToFile,
		bool bCheckDirty,
		FString& FinalFilename,
		bool bAutosaving,
		bool bPIESaving)
	{
		// SaveWorld not reentrant - check that we are not already in the process of saving here (for example, via autosave)
		static bool bIsReentrant = false;
		if (bIsReentrant)
		{
			return false;
		}

		TGuardValue<bool> ReentrantGuard(bIsReentrant, true);

		if (!World)
		{
			FinalFilename = LOCTEXT("FilenameUnavailable", "Filename Not available!").ToString();
			return false;
		}

		UPackage* Package = Cast<UPackage>(World->GetOuter());
		if (!Package)
		{
			FinalFilename = LOCTEXT("FilenameUnavailableInvalidOuter", "Filename Not available. Outer package invalid!").ToString();
			return false;
		}

		// Don't save if the world doesn't need saving.
		if (bCheckDirty && !Package->IsDirty())
		{
			FinalFilename = LOCTEXT("FilenameUnavailableNotDirty", "Filename Not available. Package not dirty.").ToString();
			return false;
		}

		FString PackageName = Package->GetName();

		FString	ExistingFilename;
		FString		Path;
		FString	CleanFilename;

		// Does a filename already exist for this package?
		const bool bPackageExists = FPackageName::DoesPackageExist(PackageName, NULL, &ExistingFilename);

		if (ForceFilename)
		{
			Path = FPaths::GetPath(*ForceFilename);
			CleanFilename = FPaths::GetCleanFilename(*ForceFilename);
		}
		else if (bPackageExists)
		{
			if (bPIESaving && FCString::Stristr(*ExistingFilename, *FPackageName::GetMapPackageExtension()) == NULL)
			{
				// If package exists, but doesn't feature the default extension, it will not load when launched,
				// Change the extension of the map to the default for the auto-save
				Path = GetAutoSaveDir();
				CleanFilename = FPackageName::GetLongPackageAssetName(PackageName) + FPackageName::GetMapPackageExtension();
			}
			else
			{
				// We're not forcing a filename, so go with the filename that exists.
				Path = FPaths::GetPath(ExistingFilename);
				CleanFilename = FPaths::GetCleanFilename(ExistingFilename);
			}
		}
		else if (!bAutosaving && FPackageName::IsValidLongPackageName(PackageName, false))
		{
			// If the package is made with a path in a non-read-only root, save it there
			const FString ImplicitFilename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetMapPackageExtension());
			Path = FPaths::GetPath(ImplicitFilename);
			CleanFilename = FPaths::GetCleanFilename(ImplicitFilename);
		}
		else
		{
			// No package filename exists and none was specified, so save the package in the autosaves folder.
			Path = GetAutoSaveDir();
			CleanFilename = FPackageName::GetLongPackageAssetName(PackageName) + FPackageName::GetMapPackageExtension();
		}

		// Optionally override path.
		if (OverridePath)
		{
			FinalFilename = FString(OverridePath) + TEXT("/");
		}
		else
		{
			FinalFilename = Path + TEXT("/");
		}

		// Apply optional filename prefix.
		if (FilenamePrefix)
		{
			FinalFilename += FString(FilenamePrefix);
		}

		// Munge remaining clean filename minus path + extension with path and optional prefix.
		FinalFilename += CleanFilename;

		// Prepare the new package name
		FString NewPackageName;
		if (!FPackageName::TryConvertFilenameToLongPackageName(FinalFilename, NewPackageName))
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::Format(NSLOCTEXT("Editor", "SaveWorld_BadFilename", "Failed to save the map. The filename '{0}' is not within the game or engine content folders found in '{1}'."), FText::FromString(FinalFilename), FText::FromString(FPaths::RootDir())));
			return false;
		}

		// Before doing any work, check to see if 1) the package name is in use by another object, 2) the world object can be renamed if necessary; and 3) the file is writable.
		bool bSuccess = false;

		const FString OriginalWorldName = World->GetName();
		const FString OriginalPackageName = Package->GetName();
		const FString NewWorldAssetName = FPackageName::GetLongPackageAssetName(NewPackageName);
		bool bValidWorldName = true;
		bool bPackageNeedsRename = false;
		bool bWorldNeedsRename = false;

		if (bRenamePackageToFile)
		{
			// Rename the world package if needed
			if (Package->GetName() != NewPackageName)
			{
				bValidWorldName = Package->Rename(*NewPackageName, NULL, REN_Test);
				if (bValidWorldName)
				{
					bPackageNeedsRename = true;
				}
			}

			if (bValidWorldName)
			{
				// Rename the world if the package changed
				if (World->GetName() != NewWorldAssetName)
				{
					bValidWorldName = World->Rename(*NewWorldAssetName, NULL, REN_Test);
					if (bValidWorldName)
					{
						bWorldNeedsRename = true;
					}
				}
			}
		}

		if (!bValidWorldName)
		{
			FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("UnrealEd", "Error_LevelNameExists", "A level with that name already exists. Please choose another name."));
		}
		else if (IFileManager::Get().IsReadOnly(*FinalFilename))
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::Format(NSLOCTEXT("UnrealEd", "PackageFileIsReadOnly", "Unable to save package to {0} because the file is read-only!"), FText::FromString(FinalFilename)));
		}
		else
		{
			// Save the world package after doing optional garbage collection.
			const FScopedBusyCursor BusyCursor;

			FFormatNamedArguments Args;
			Args.Add(TEXT("MapFilename"), FText::FromString(FPaths::GetCleanFilename(FinalFilename)));

			FScopedSlowTask SlowTask(100, FText::Format(NSLOCTEXT("UnrealEd", "SavingMap_F", "Saving map: {MapFilename}..."), Args));
			SlowTask.MakeDialog(true);

			SlowTask.EnterProgressFrame(25);

			FSoftObjectPath OldPath(World);
			bool bAddedAssetPathRedirection = false;

			// Rename the package and the object, as necessary
			UWorld* DuplicatedWorld = nullptr;
			if (bRenamePackageToFile)
			{
				if (bPackageNeedsRename)
				{
					// If we are doing a SaveAs on a world that already exists, we need to duplicate it.
					if (bPackageExists)
					{
						ObjectTools::FPackageGroupName NewPGN;
						NewPGN.PackageName = NewPackageName;
						NewPGN.ObjectName = NewWorldAssetName;

						bool bPromptToOverwrite = false;
						TSet<UPackage*> PackagesUserRefusedToFullyLoad;
						DuplicatedWorld = Cast<UWorld>(ObjectTools::DuplicateSingleObject(World, NewPGN, PackagesUserRefusedToFullyLoad, bPromptToOverwrite));
						if (DuplicatedWorld)
						{
							Package = DuplicatedWorld->GetOutermost();
						}
					}

					if (!DuplicatedWorld)
					{
						// Duplicate failed or not needed. Just do a rename.
						Package->Rename(*NewPackageName, NULL, REN_NonTransactional | REN_DontCreateRedirectors | REN_ForceNoResetLoaders);

						if (bWorldNeedsRename)
						{
							// Unload package of existing MapBuildData to allow overwrite
							if (World->PersistentLevel->MapBuildData && !World->PersistentLevel->MapBuildData->IsLegacyBuildData())
							{
								FString NewBuiltPackageName = World->GetOutermost()->GetName() + TEXT("_BuiltData");
								UObject* ExistingObject = StaticFindObject(nullptr, 0, *NewBuiltPackageName);
								if (ExistingObject && ExistingObject != World->PersistentLevel->MapBuildData->GetOutermost())
								{
									TArray<UPackage*> AllPackagesToUnload;
									AllPackagesToUnload.Add(Cast<UPackage>(ExistingObject));
									UPackageTools::UnloadPackages(AllPackagesToUnload);
								}
							}

							World->Rename(*NewWorldAssetName, NULL, REN_NonTransactional | REN_DontCreateRedirectors | REN_ForceNoResetLoaders);

							// We're renaming the world, add a path redirector so that soft object paths get fixed on save
							FSoftObjectPath NewPath(World);
							GRedirectCollector.AddAssetPathRedirection(*OldPath.GetAssetPathString(), *NewPath.GetAssetPathString());
							bAddedAssetPathRedirection = true;
						}
					}
				}
			}

			SlowTask.EnterProgressFrame(50);

			// Save package.
			{
				const FString AutoSavingString = (bAutosaving || bPIESaving) ? TEXT("true") : TEXT("false");
				const FString KeepDirtyString = bPIESaving ? TEXT("true") : TEXT("false");
				FSaveErrorOutputDevice SaveErrors;

				bSuccess = GEditor->Exec(NULL, *FString::Printf(TEXT("OBJ SAVEPACKAGE PACKAGE=\"%s\" FILE=\"%s\" SILENT=true AUTOSAVING=%s KEEPDIRTY=%s"), *Package->GetName(), *FinalFilename, *AutoSavingString, *KeepDirtyString), SaveErrors);
				SaveErrors.Flush();
			}

			if (bSuccess && !bAutosaving)
			{
				// Also save MapBuildData packages when saving the current level
				FEditorFileUtils::SaveMapDataPackages(DuplicatedWorld ? DuplicatedWorld : World, bCheckDirty || bPIESaving);
			}

			SlowTask.EnterProgressFrame(25);

			// If the package save was not successful. Trash the duplicated world or rename back if the duplicate failed.
			if (bRenamePackageToFile && !bSuccess)
			{
				if (bPackageNeedsRename)
				{
					if (DuplicatedWorld)
					{
						DuplicatedWorld->Rename(nullptr, GetTransientPackage(), REN_NonTransactional | REN_DontCreateRedirectors);
						DuplicatedWorld->MarkPendingKill();
						DuplicatedWorld->SetFlags(RF_Transient);
						DuplicatedWorld = nullptr;
					}
					else
					{
						Package->Rename(*OriginalPackageName, NULL, REN_NonTransactional);

						if (bWorldNeedsRename)
						{
							World->Rename(*OriginalWorldName, NULL, REN_NonTransactional);
						}
					}
				}
			}
		}

		return bSuccess;
	}

	/**
	 * Saves the specified map package, returning true on success.
	 *
	 * @param	World			The world to save.
	 * @param	Filename		Map package filename, including path.
	 *
	 * @return					true if the map was saved successfully.
	 */
	static bool InternalSaveMap(UWorld* InWorld, const FString& Filename)
	{
		bool bLevelWasSaved = false;

		// Disallow the save if in interpolation editing mode and the user doesn't want to exit interpolation mode.
		if (!InInterpEditMode())
		{
			const double SaveStartTime = FPlatformTime::Seconds();

			FString FinalFilename;
			bLevelWasSaved = InternalSaveWorld(InWorld, &Filename,
				nullptr, nullptr,
				true, false,
				FinalFilename,
				false, false);

			// Track time spent saving map.
			UE_LOG(LogSoEditor, Log, TEXT("Saving map '%s' took %.3f"), *FPaths::GetBaseFilename(Filename), FPlatformTime::Seconds() - SaveStartTime);
		}

		return bLevelWasSaved;
	}

	/**
	 * Actually save a package. Prompting for Save as if necessary
	 *
	 * @param PackageToSave					The package to save.
	 * @param bUseDialog					If true, use the normal behavior.
	 *										If false, do not prompt message dialog. If it can't save the package, skip it. If the package is a map and the name is not valid, skip it.
	 * @param OutPackageLocallyWritable		Set to true if the provided package was locally writable but not under source control (of if source control is disabled).
	 * @param SaveOutput					The output from the save process.
	 * @return	InternalSavePackageResult::Success if package saving was a success
				InternalSavePackageResult::Continue if the package saving failed and the user doesn't want to retry
				InternalSavePackageResult::Cancel if the user wants to cancel everything
				InternalSavePackageResult::Error if an error occured. Check OutFailureReason
	 */
	static InternalSavePackageResult InternalSavePackage(UPackage* PackageToSave, bool bUseDialog, bool& bOutPackageLocallyWritable, FOutputDevice &SaveOutput)
	{
		// What we will be returning. Assume for now that everything will go fine
		InternalSavePackageResult ReturnCode = InternalSavePackageResult::Success;

		// Assume the package is locally writable in case SCC is disabled; if SCC is enabled, it will
		// correctly set this value later
		bOutPackageLocallyWritable = true;

		bool bShouldRetrySave = true;
		UWorld*	AssociatedWorld = UWorld::FindWorldInPackage(PackageToSave);
		const bool	bIsMapPackage = AssociatedWorld != NULL;

		// The name of the package
		const FString PackageName = PackageToSave->GetName();

		// Place were we should save the file, including the filename
		FString FinalPackageSavePath;
		// Just the filename
		FString FinalPackageFilename;

		// True if we should attempt saving
		bool bAttemptSave = true;

		// If the package already has a valid path to a non read-only location, use it to determine where the file should be saved
		const bool bIncludeReadOnlyRoots = false;
		const bool bIsValidPath = FPackageName::IsValidLongPackageName(PackageName, bIncludeReadOnlyRoots);
		if (bIsValidPath)
		{
			FString ExistingFilename;
			const bool bPackageAlreadyExists = FPackageName::DoesPackageExist(PackageName, NULL, &ExistingFilename);
			if (!bPackageAlreadyExists)
			{
				// Construct a filename from long package name.
				const FString& FileExtension = bIsMapPackage ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension();
				ExistingFilename = FPackageName::LongPackageNameToFilename(PackageName, FileExtension);

				// Check if we can use this filename.
				FText ErrorText;
				if (!FFileHelper::IsFilenameValidForSaving(ExistingFilename, ErrorText))
				{
					// Display the error (already localized) and exit gracefuly.
					FMessageDialog::Open(EAppMsgType::Ok, ErrorText);
					bAttemptSave = false;
				}
			}

			if (bAttemptSave)
			{
				// The file already exists, no need to prompt for save as
				FString BaseFilename, Extension, Directory;
				// Split the path to get the filename without the directory structure
				FPaths::NormalizeFilename(ExistingFilename);
				FPaths::Split(ExistingFilename, Directory, BaseFilename, Extension);
				// The final save path is whatever the existing filename is
				FinalPackageSavePath = ExistingFilename;
				// Format the filename we found from splitting the path
				FinalPackageFilename = FString::Printf(TEXT("%s.%s"), *BaseFilename, *Extension);
			}
		}
		else if (bUseDialog && bIsMapPackage)	// don't do a SaveAs dialog if dialogs was not requested
		{
			// If this changes, there must be generic code to rename assets to the new name BEFORE saving to disk.
			// Right now, all of this code is specific to maps

			// There wont be a "not checked out from SCC but writable on disk" conflict if the package is new.
			bOutPackageLocallyWritable = false;

			// Make a list of file types
			// We have to ask for save as.
			FString FileTypes;
			FText SavePackageText;

			if (bIsMapPackage)
			{
				FileTypes = FEditorFileUtils::GetFilterString(FI_Save);
				FinalPackageFilename = FString::Printf(TEXT("Untitled%s"), *FPackageName::GetMapPackageExtension());
				SavePackageText = NSLOCTEXT("UnrealEd", "SaveMap", "Save Map");
			}
			else
			{
				FileTypes = FString::Printf(TEXT("(*%s)|*%s"), *FPackageName::GetAssetPackageExtension(), *FPackageName::GetAssetPackageExtension());
				FinalPackageFilename = FString::Printf(TEXT("%s%s"), *PackageToSave->GetName(), *FPackageName::GetAssetPackageExtension());
				SavePackageText = NSLOCTEXT("UnrealEd", "SaveAsset", "Save Asset");
			}

			// The number of times the user pressed cancel
			int32 NumSkips = 0;

			// If the user presses cancel more than this time, they really don't want to save the file
			const int32 NumSkipsBeforeAbort = 1;

			// if the user hit cancel on the Save dialog, ask again what the user wants to do,
			// we shouldn't assume they want to skip the file
			// This loop continues indefinitely if the user does not supply a valid filename.  They must supply a valid filename or press cancel
			const FString Directory = *GetDefaultDirectory();
			while (NumSkips < NumSkipsBeforeAbort)
			{
				FString DefaultLocation = Directory;
				FString DefaultPackagePath;
				if (!FPackageName::TryConvertFilenameToLongPackageName(DefaultLocation / FinalPackageFilename, DefaultPackagePath))
				{
					// Original location is invalid; set default location to /Game/Maps
					DefaultLocation = FPaths::ProjectContentDir() / TEXT("Maps");
					ensure(FPackageName::TryConvertFilenameToLongPackageName(DefaultLocation / FinalPackageFilename, DefaultPackagePath));
				}

				FString SaveAsPackageName;
				bool bSaveFile = OpenSaveAsDialog(
					UWorld::StaticClass(),
					FPackageName::GetLongPackagePath(DefaultPackagePath),
					FPaths::GetBaseFilename(FinalPackageFilename),
					SaveAsPackageName);

				if (bSaveFile)
				{
					// Leave out the extension. It will be added below.
					FinalPackageFilename = FPackageName::LongPackageNameToFilename(SaveAsPackageName);
				}

				if (bSaveFile)
				{
					// If the supplied file name is missing an extension then give it the default package
					// file extension.
					if (FinalPackageFilename.Len() > 0 && FPaths::GetExtension(FinalPackageFilename).Len() == 0)
					{
						FinalPackageFilename += bIsMapPackage ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension();
					}

					FText ErrorMessage;
					bool bValidFilename = FFileHelper::IsFilenameValidForSaving(FinalPackageFilename, ErrorMessage);
					if (bValidFilename)
					{
						bValidFilename = bIsMapPackage ? FEditorFileUtils::IsValidMapFilename(FinalPackageFilename, ErrorMessage) : FPackageName::IsValidLongPackageName(FinalPackageFilename, false, &ErrorMessage);
					}

					if (bValidFilename)
					{
						// If there is an existing world in memory that shares this name unload it now to prepare for overwrite.
						// Don't do this if we are using save as to overwrite the current level since it will just save naturally.
						const FString NewPackageName = FPackageName::FilenameToLongPackageName(FinalPackageFilename);
						UPackage* ExistingPackage = FindPackage(nullptr, *NewPackageName);
						if (ExistingPackage && ExistingPackage != PackageToSave)
						{
							bValidFilename = FEditorFileUtils::AttemptUnloadInactiveWorldPackage(ExistingPackage, ErrorMessage);
						}
					}

					if (!bValidFilename)
					{
						// Start the loop over, prompting for save again
						const FText DisplayFilename = FText::FromString(IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*FinalPackageFilename));
						FFormatNamedArguments Arguments;
						Arguments.Add(TEXT("Filename"), DisplayFilename);
						Arguments.Add(TEXT("LineTerminators"), FText::FromString(LINE_TERMINATOR LINE_TERMINATOR));
						Arguments.Add(TEXT("ErrorMessage"), ErrorMessage);
						const FText DisplayMessage = FText::Format(LOCTEXT("InvalidSaveFilename", "Failed to save to {Filename}{LineTerminators}{ErrorMessage}"), Arguments);
						FMessageDialog::Open(EAppMsgType::Ok, DisplayMessage);

						// Start the loop over, prompting for save again
						continue;
					}
					else
					{
						FinalPackageSavePath = FinalPackageFilename;
						// Stop looping, we successfully got a valid path and filename to save
						break;
					}
				}
				else
				{
					// if the user hit cancel on the Save dialog, ask again what the user wants to do,
					// we shouldn't assume they want to skip the file unless they press cancel several times
					++NumSkips;
					if (NumSkips == NumSkipsBeforeAbort)
					{
						// They really want to stop
						bAttemptSave = false;
						ReturnCode = InternalSavePackageResult::Cancel;
					}
				}
			}
		}

		// attempt the save

		while (bAttemptSave)
		{
			bool bWasSuccessful = false;
			if (bIsMapPackage)
			{
				// have a Helper attempt to save the map
				SaveOutput.Log("LogFileHelpers", ELogVerbosity::Log, FString::Printf(TEXT("Saving Map: %s"), *PackageName));
				bWasSuccessful = InternalSaveMap(AssociatedWorld, FinalPackageSavePath);
			}
			else
			{
				// normally, we just save the package
				SaveOutput.Log("LogFileHelpers", ELogVerbosity::Log, FString::Printf(TEXT("Saving Package: %s"), *PackageName));
				bWasSuccessful = GEngine->Exec(NULL, *FString::Printf(TEXT("OBJ SAVEPACKAGE PACKAGE=\"%s\" FILE=\"%s\" SILENT=true"), *PackageName, *FinalPackageSavePath), SaveOutput);
			}

			ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
			if (ISourceControlModule::Get().IsEnabled())
			{
				// Assume the package was correctly checked out from SCC
				bOutPackageLocallyWritable = false;

				// Trusting the SCC status in the package file cache to minimize network activity during save.
				const FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(PackageToSave, EStateCacheUsage::Use);
				// If the package is in the depot, and not recognized as editable by source control, and not read-only, then we know the user has made the package locally writable!
				const bool bSCCCanEdit = !SourceControlState.IsValid() || SourceControlState->CanCheckIn() || SourceControlState->IsIgnored() || SourceControlState->IsUnknown();
				const bool bSCCIsCheckedOut = SourceControlState.IsValid() && SourceControlState->IsCheckedOut();
				const bool bInDepot = SourceControlState.IsValid() && SourceControlState->IsSourceControlled();
				if (!bSCCCanEdit && bInDepot && !IFileManager::Get().IsReadOnly(*FinalPackageSavePath) && SourceControlProvider.UsesLocalReadOnlyState() && !bSCCIsCheckedOut)
				{
					bOutPackageLocallyWritable = true;
				}
			}
			else
			{
				// If source control is disabled then we don't care if the package is locally writable
				bOutPackageLocallyWritable = false;
			}

			// Handle all failures the same way.
			if (bUseDialog && !bWasSuccessful)
			{
				// ask the user what to do if we failed
				const FText ErrorPrompt = GEditor->IsPlayingOnLocalPCSession() ?
					NSLOCTEXT("UnrealEd", "Prompt_41", "The asset '{0}' ({1}) cannot be saved as the package is locked because you are in play on PC mode.\n\nCancel: Stop saving all assets and return to the editor.\nRetry: Attempt to save the asset again.\nContinue: Skip saving this asset only.") :
					NSLOCTEXT("UnrealEd", "Prompt_26", "The asset '{0}' ({1}) failed to save.\n\nCancel: Stop saving all assets and return to the editor.\nRetry: Attempt to save the asset again.\nContinue: Skip saving this asset only.");
				EAppReturnType::Type DialogCode = FMessageDialog::Open(EAppMsgType::CancelRetryContinue, EAppReturnType::Continue, FText::Format(ErrorPrompt, FText::FromString(PackageName), FText::FromString(FinalPackageFilename)));

				switch (DialogCode)
				{
				case EAppReturnType::Cancel:
					// if this happens, the user wants to stop everything
					bAttemptSave = false;
					ReturnCode = InternalSavePackageResult::Cancel;
					break;
				case EAppReturnType::Retry:
					bAttemptSave = true;
					break;
				case EAppReturnType::Continue:
					ReturnCode = InternalSavePackageResult::Continue;// this is if it failed to save, but the user wants to skip saving it
					bAttemptSave = false;
					break;
				default:
					// Should not get here
					check(0);
					break;
				}
			}
			else if (!bWasSuccessful)
			{
				// We failed at saving because we are in bIsUnattended mode, there is no need to attempt to save again
				FText FailureReason = FText::Format(NSLOCTEXT("UnrealEd", "SaveAssetFailed", "The asset '{0}' ({1}) failed to save."), FText::FromString(PackageName), FText::FromString(FinalPackageFilename));

				UE_LOG(LogSoEditor, Error, TEXT("%s"), *FailureReason.ToString());
				if (bUseDialog)
				{
					FMessageDialog::Open(EAppMsgType::Ok, FailureReason);
				}

				bAttemptSave = false;
				ReturnCode = InternalSavePackageResult::Error;
			}
			else
			{
				// If we were successful at saving, there is no need to attempt to save again
				bAttemptSave = false;
				ReturnCode = InternalSavePackageResult::Success;
			}
		}

		return ReturnCode;
	}

	/*
	 * @param bUseDialog					If true, use the normal behavior.
	 *										If false, do not prompt message dialog. If it can't save the package, skip it. If the package is a map and the name is not valid, skip it.
	 * @param	bShowDialogIfError			If InternalSavePackage failed, tell the user with a Dialog
	 * @param	OutFailedPackages			Packages that failed to save
	 */
	static bool InternalSavePackagesFast(const TArray<UPackage*>& PackagesToSave, bool bUseDialog, TArray<UPackage*>& OutFailedPackages)
	{
		bool bReturnCode = true;

		FSaveErrorOutputDevice SaveErrors;
		GWarn->BeginSlowTask(NSLOCTEXT("UnrealEd", "SavingPackagesE", "Saving packages..."), true);

		for (TArray<UPackage*>::TConstIterator PkgIter(PackagesToSave); PkgIter; ++PkgIter)
		{
			UPackage* CurPackage = *PkgIter;

			// Check if a file exists for this package
			FString Filename;
			const bool bFoundFile = FPackageName::DoesPackageExist(CurPackage->GetName(), NULL, &Filename);
			if (bFoundFile)
			{
				// determine if the package file is read only
				const bool bPkgReadOnly = IFileManager::Get().IsReadOnly(*Filename);

				// Only save writable files in fast mode
				if (!bPkgReadOnly)
				{
					if (!CurPackage->IsFullyLoaded())
					{
						// Packages must be fully loaded to save
						CurPackage->FullyLoad();
					}

					const UWorld* const AssociatedWorld = UWorld::FindWorldInPackage(CurPackage);
					const bool bIsMapPackage = AssociatedWorld != nullptr;

					const FText SavingPackageText = (bIsMapPackage)
						? FText::Format(NSLOCTEXT("UnrealEd", "SavingMapf", "Saving map {0}"), FText::FromString(CurPackage->GetName()))
						: FText::Format(NSLOCTEXT("UnrealEd", "SavingAssetf", "Saving asset {0}"), FText::FromString(CurPackage->GetName()));

					GWarn->StatusForceUpdate(PkgIter.GetIndex(), PackagesToSave.Num(), SavingPackageText);

					// Save the package
					bool bPackageLocallyWritable;
					const InternalSavePackageResult SaveStatus = InternalSavePackage(CurPackage, bUseDialog, bPackageLocallyWritable, SaveErrors);

					if (SaveStatus == InternalSavePackageResult::Cancel)
					{
						// we don't want to pop up a message box about failing to save packages if they cancel
						// instead warn here so there is some trace in the log and also unattended builds can find it
						UE_LOG(LogSoEditor, Warning, TEXT("Cancelled saving package %s"), *CurPackage->GetName());
					}
					else if (SaveStatus == InternalSavePackageResult::Continue || SaveStatus == InternalSavePackageResult::Error)
					{
						// The package could not be saved so add it to the failed array
						OutFailedPackages.Add(CurPackage);

						if (SaveStatus == InternalSavePackageResult::Error)
						{
							// exit gracefully.
							bReturnCode = false;
						}
					}
				}
			}
		}

		GWarn->EndSlowTask();
		SaveErrors.Flush();

		return bReturnCode;
	}

	static bool InternalSavePackages(const TArray<UPackage*>& PackagesToSave, const bool bUseDialog)
	{
		TArray<UPackage*> FailedPackages;
		const bool bReturnCode = InternalSavePackagesFast(PackagesToSave, bUseDialog, FailedPackages);

		// Warn the user about any packages which failed to save.
		InternalWarnUserAboutFailedSave(FailedPackages, bUseDialog);


		return bReturnCode;
	}

	static bool SaveDirtyPackages(
		const bool bSaveMapPackages,
		const bool bSaveContentPackages,
		const bool bUseDialog,
		const bool bNotifyNoPackagesSaved = false,
		bool* bOutPackagesNeededSaving = nullptr)
	{
		bool bReturnCode = true;

		if (bOutPackagesNeededSaving != NULL)
		{
			*bOutPackagesNeededSaving = false;
		}

		const TArray<UPackage*> PackagesToSave = InternalGetDirtyPackages(bSaveMapPackages, bSaveContentPackages);
		if (PackagesToSave.Num() > 0)
		{
			if (bOutPackagesNeededSaving != nullptr)
			{
				*bOutPackagesNeededSaving = true;
			}

			bReturnCode = InternalSavePackages(PackagesToSave, bUseDialog);
		}
		else if (bNotifyNoPackagesSaved)
		{
			InternalNotifyNoPackagesSaved(true);
		}
		return bReturnCode;
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// End own save dirty packages
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
};

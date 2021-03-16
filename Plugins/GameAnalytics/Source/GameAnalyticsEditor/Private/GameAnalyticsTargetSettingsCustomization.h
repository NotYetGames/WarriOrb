// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Input/SComboBox.h"
#include "PropertyEditorModule.h"
#include "GameAnalyticsProjectSettings.h"
#include "IDetailCustomization.h"

#include "Http.h"

//////////////////////////////////////////////////////////////////////////
// FAndroidTargetSettingsCustomization

class FGameAnalyticsTargetSettingsCustomization : public IDetailCustomization
{
public:

	struct GAME {
		FString Name;
		int Id;
		FString GameKey;
		FString SecretKey;
	};

	struct STUDIO {
		FString Name;
		int Id;
		TArray<GAME> Games;

		/*STUDIO (FString aName, int aId, TArray<GAME> aGames) :
		Name(aName), Id(aId), Games(aGames)
		{
		}*/
	};

	// Makes a new instance of this detail layout class for a specific detail view requesting it
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	// End of IDetailCustomization interface

	void UsernameEntered(const FText& NewText, ETextCommit::Type CommitInfo);
	void UsernameChanged(const FText& NewText);

	void PasswordEntered(const FText& NewText, ETextCommit::Type CommitInfo);
	void PasswordChanged(const FText& NewText);

	void FillStudiosString(TArray<STUDIO> StudioList);
	TSharedRef<SWidget> HandleStudiosComboBoxGenerateWidget(TSharedPtr<FString> StringItem);
	void HandleStudiosComboBoxSelectionChanged(TSharedPtr<FString> StringItem, ESelectInfo::Type SelectInfo);
	FText HandleStudiosComboBoxContentText() const;

	void OpenStudioAndGameSelector();

	void OnStudioMenuItemClickedIos(FGameAnalyticsTargetSettingsCustomization::STUDIO StudioItem);
	void OnGameMenuItemClickedIos(FGameAnalyticsTargetSettingsCustomization::GAME GameItem);
	void OnStudioMenuItemClickedAndroid(FGameAnalyticsTargetSettingsCustomization::STUDIO StudioItem);
	void OnGameMenuItemClickedAndroid(FGameAnalyticsTargetSettingsCustomization::GAME GameItem);
    void OnStudioMenuItemClickedMac(FGameAnalyticsTargetSettingsCustomization::STUDIO StudioItem);
    void OnGameMenuItemClickedMac(FGameAnalyticsTargetSettingsCustomization::GAME GameItem);
	void OnStudioMenuItemClickedWindows(FGameAnalyticsTargetSettingsCustomization::STUDIO StudioItem);
    void OnGameMenuItemClickedWindows(FGameAnalyticsTargetSettingsCustomization::GAME GameItem);
    void OnStudioMenuItemClickedLinux(FGameAnalyticsTargetSettingsCustomization::STUDIO StudioItem);
    void OnGameMenuItemClickedLinux(FGameAnalyticsTargetSettingsCustomization::GAME GameItem);
    void OnStudioMenuItemClickedHtml5(FGameAnalyticsTargetSettingsCustomization::STUDIO StudioItem);
    void OnGameMenuItemClickedHtml5(FGameAnalyticsTargetSettingsCustomization::GAME GameItem);

public:
	static FGameAnalyticsTargetSettingsCustomization& getInstance()
	{
		static FGameAnalyticsTargetSettingsCustomization instance;  // Guaranteed to be destroyed.
		// Instantiated on first use.
		return instance;
	}

	TArray<STUDIO> StudiosAndGames;
	STUDIO SelectedStudioIos;
	GAME SelectedGameIos;
	STUDIO SelectedStudioAndroid;
	GAME SelectedGameAndroid;
    STUDIO SelectedStudioMac;
    GAME SelectedGameMac;
	STUDIO SelectedStudioWindows;
    GAME SelectedGameWindows;
    STUDIO SelectedStudioLinux;
    GAME SelectedGameLinux;
    STUDIO SelectedStudioHtml5;
    GAME SelectedGameHtml5;

private:
	FGameAnalyticsTargetSettingsCustomization() {};

	FGameAnalyticsTargetSettingsCustomization(FGameAnalyticsTargetSettingsCustomization const&) = delete;
	void operator=(FGameAnalyticsTargetSettingsCustomization const&) = delete;

	FReply OpenSetupWizard();
	FReply Login();
	TSharedRef<SWidget> UpdateStudiosIos() const;
	TSharedRef<SWidget> UpdateGamesIos() const;
	TSharedRef<SWidget> UpdateStudiosAndroid() const;
	TSharedRef<SWidget> UpdateGamesAndroid() const;
    TSharedRef<SWidget> UpdateStudiosMac() const;
    TSharedRef<SWidget> UpdateGamesMac() const;
	TSharedRef<SWidget> UpdateStudiosWindows() const;
    TSharedRef<SWidget> UpdateGamesWindows() const;
    TSharedRef<SWidget> UpdateStudiosLinux() const;
    TSharedRef<SWidget> UpdateGamesLinux() const;
    TSharedRef<SWidget> UpdateStudiosHtml5() const;
    TSharedRef<SWidget> UpdateGamesHtml5() const;

    FORCEINLINE FString GetIniName() const { return FString::Printf(TEXT("%sDefaultEngine.ini"), *FPaths::SourceConfigDir()); }

private:
	IDetailLayoutBuilder* SavedLayoutBuilder;

	FHttpModule* Http;
	bool bIsBusy;

	FName Username;
	FName Password;

	TSharedPtr<SComboBox< TSharedPtr<FString> >>		StudiosComboBox;
	TArray< TSharedPtr<FString> >						StudiosComboBoxString;
};

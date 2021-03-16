#include "GameAnalyticsEditor.h"

#include "GameAnalyticsTargetSettingsCustomization.h"

#include "Modules/ModuleInterface.h"
#include "ISettingsModule.h"
#include "Modules/ModuleManager.h"

#include "GameAnalyticsProjectSettings.h"

#define LOCTEXT_NAMESPACE "GameAnalyticsPlugin"

void FGameAnalyticsEditor::StartupModule()
{
    // register settings detail panel customization
    FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    PropertyModule.RegisterCustomClassLayout(
                                             UGameAnalyticsProjectSettings::StaticClass()->GetFName(),
                                             FOnGetDetailCustomizationInstance::CreateStatic(&FGameAnalyticsTargetSettingsCustomization::MakeInstance)
                                             );

    PropertyModule.NotifyCustomizationModuleChanged();

    ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
    if( SettingsModule != nullptr )
    {
        SettingsModule->RegisterSettings(
                                         "Project", "Plugins", "GameAnalytics",
                                         LOCTEXT( "GameAnalyticsSettingsName", "GameAnalytics" ),
                                         LOCTEXT( "GameAnalyticsSettingsDescription", "GameAnalytics settings" ),
                                         GetMutableDefault< UGameAnalyticsProjectSettings >() );
    }
}

void FGameAnalyticsEditor::ShutdownModule()
{
    ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
    if( SettingsModule != nullptr )
    {
        SettingsModule->UnregisterSettings( "Project", "Plugins", "GameAnalytics" );
    }
}

IMPLEMENT_MODULE(FGameAnalyticsEditor, GameAnalyticsEditor)

#undef LOCTEXT_NAMESPACE

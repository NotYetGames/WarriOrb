# Analytics

The default settings values for this are in [`Config/DefaultEngine.ini`](Config/DefaultEngine.ini)


## Links
- [UE Docs](https://docs.unrealengine.com/en-US/Gameplay/Analytics/index.html)
- [Analytics Provider Website](https://gameanalytics.com/)
- Plugin Location - /Plugins/GameAnalytics
- [Repository SDK Unreal](https://github.com/vampy/GA-SDK-UNREAL)
- [Repository SDK CPP](https://github.com/vampy/GA-SDK-CPP)

## Settings `<Project>/Saved/<Target>/Engine.ini`

To disable only GameAnalytics:
```ini
[Analytics]
ProviderModuleName=AnalyticsMulticast
ProviderModuleNames=FileLogging

[AnalyticsTest]
ProviderModuleName=AnalyticsMulticast
ProviderModuleNames=FileLogging

[AnalyticsDebug]
ProviderModuleName=AnalyticsMulticast
ProviderModuleNames=FileLogging

[AnalyticsDevelopment]
ProviderModuleName=AnalyticsMulticast
ProviderModuleNames=FileLogging
```

To enable both file logging and GameAnalytics:
```ini
[Analytics]
ProviderModuleName=AnalyticsMulticast
ProviderModuleNames=FileLogging,GameAnalytics

[AnalyticsTest]
ProviderModuleName=AnalyticsMulticast
ProviderModuleNames=FileLogging,GameAnalytics

[AnalyticsDebug]
ProviderModuleName=AnalyticsMulticast
ProviderModuleNames=FileLogging,GameAnalytics

[AnalyticsDevelopment]
ProviderModuleName=AnalyticsMulticast
ProviderModuleNames=FileLogging,GameAnalytics
```

To reset to default from engine:
```ini
[Analytics]
UE4TypeOverride="Rocket"
```

## Settings `<Project>/Saved/<Target>/GameUserSettings.ini`

To disable the complete collection of analytics
```ini
# Must be under this category
[/Script/SOrb.SoGameSettings]
bCollectGameAnalytics=False
bWaitForAnalyticsToSend=False
```

# Crashes
NOTE: To create a fake crash, type this into the console in game: `debug crash`

NOTE: To force log callstacks you can add `-ForceLogCallstacks` to the command line.

Steps:
1. Go to `Saved/Crashes/<Crash Folder>`
2. You can ignore the log file from the `Crashes` folder as it is not complete, check instead `Saved/Logs/<Log file>` this will have the call stack at the end.
3. Open the project in Visual Studio then open the `UE4Minidump.dmp` file.
4. Start debugging it via clicking on `Debug with Native Only`


## Crash Reporter Client

[**See how the crash reporter reads the config files**](https://forum.sentry.io/t/unreal-engine-crash-reporter-cant-get-it-to-work/7643/10)

NOTE: We also handle our own crashes inside `FSoCrashHandler`

See Docs:
- https://docs.sentry.io/platforms/native/ue4/
- https://forum.sentry.io/t/unreal-engine-crash-reporter-cant-get-it-to-work/7643

## Copy crash Config from project to Engine

This copies the `[CrashReportClient]` section from config `<ProjectPath>/Config/DefaultEngine.ini` to `<EnginePath>/Engine/Programs/CrashReportClient/Config/DefaultEngine.ini`

```sh
./Tools/UnrealEngine/CopyCrashReporterSettingsToEngine.py
```

## CLI and debug files

- https://docs.sentry.io/workflow/debug-files/
- https://docs.sentry.io/cli/dif/#uploading-files
- https://docs.sentry.io/cli

```sh
# Install sentry-cli
./Tools/setup-entry.sh

# Set auth token, get it from https://sentry.io/settings/account/api/auth-tokens/
# Modify ./Tools/config.ini SENTRY_AUTH_TOKEN

# Run
./Tools/SentryCLI.py


### Debug files
./Tools/Warriorb/Demo_Sentry_UploadDif.py <debug files or folders>
```

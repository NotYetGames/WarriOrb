# Command line arguments

When running the game you can append these arguments:
```markdown
#
# Warriorb specific
#
-monitor=    - Select a monitor, indexing starts from 1 to M (number of monitors).
               Usually the same order as the display numbers in windows/linux.
               If -monitor=0 it selects the primary desktop monitor.

-CollectGameAnalytics | -NoCollectGameAnalytics     - Enable/Disable collecting analytics (override for bCollectGameAnalytics from GameUserSettings.ini)
-PauseGameWhenUnfocused | -NoPauseGameWhenUnfocused - Enable/Disable pausing when unfocused (override for bPauseGameWhenUnfocused from GameUserSettings.ini)
-WaitForAnalyticsToSend | -NoWaitForAnalyticsToSend - Enables/Disable waiting for analytics to send (override for bWaitForAnalyticsToSend from GameUserSettings.ini)

-NoHardwareBenchmark - Do not run any hardware benchmark on first startup, useful for low powered devices like intel cards

-NoGPUBlacklistBox - Disables the message popup for blacklisted GPUs, useful for low end GPUs like intel cards

-CheatsPassword="<password from WarriorbBuild.json>" - enable cheats at runtime

-SpeedRunCompetition - Enables the speed run competition mode, checks the checksum and that the time is correct.

-AssetsPrintAll     - Prints in the log all assets
-AssetsPrintAllMaps - Prints in the log all assets maps

-GAInfoLogBuild | -GANoInfoLogBuild - enable/disable GameAnalytics info logs
-GAVerboseLogBuild | - GANoVerboseLogBuild - enable/disable GameAnalytics verbose logs
```

## Editor: [See snippet](https://gitlab.com/snippets/1834336#command-line-arguments)

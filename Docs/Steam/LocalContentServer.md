# Local Content Server upload

> **Note:**
This is for testing only, it is a local web server on your machine, no data is pushed to steam.

Main app build file: [`Steam/game/app_build_Warriorb_790360_Local.vdf`](/Steam/game/app_build_Warriorb_790360_Local.vdf)


## Config File

See https://partner.steamgames.com/doc/sdk/uploading/local_content_server for details.


> **Note:**
[You MUST Configuring Steam Client To Use Your Local Content Server.](https://partner.steamgames.com/doc/sdk/uploading/local_content_server#3)


### WarriOrb

You need to copy [`Steam/game/app_build_Warriorb_790360_Local.EXAMPLE.vdf`](/Steam/game/app_build_Warriorb_790360_Local.EXAMPLE.vdf) to [`Steam/game/app_build_Warriorb_790360_Local.vdf`](/Steam/game/app_build_Warriorb_790360_Local.vdf) and modify for your local configuration.
configuration.

### WarriOrb: Prologue

You need to copy [`Steam/demo/app_build_Warriorb_demo_1193760_Local.EXAMPLE.vdf`](/Steam/demo/app_build_Warriorb_demo_1193760_Local.EXAMPLE.vdf) to [`Steam/demo/app_build_Warriorb_demo_1193760_Local.vdf`](/Steam/demo/app_build_Warriorb_demo_1193760_Local.vdf) and modify for your local


## Configure paths `htdocs` path:
1. In `<STEAM_VERSION_SDK_PATH>/tools/ContentServer/mongoose.conf` edit `document_root`
2. In [`Steam/app_build_Warriorb_790360_Local.vdf`](Steam/app_build_Warriorb_790360_Local.vdf) edit `local` to be the same as the value from `1`

## Run

1. Start web server by running `<STEAM_VERSION_SDK_PATH>/tools/ContentServer/mongoose-3.1.exe`

2. Package

### WarriOrb

```sh
# Package game
./Tools/Warriorb/Steam_PackageWindows.py

# Upload to local server
./Tools/SteamUpload.py ./Steam/game/app_build_Warriorb_790360_Local.vdf
```

### WarriOrb: Prologue
```sh
# Package game
./Tools/Warriorb/Demo_Steam_PackageWindows.py

# Upload to local server
./Tools/SteamUpload.py ./Steam/demo/app_build_Warriorb_demo_1193760_Local.vdf
```

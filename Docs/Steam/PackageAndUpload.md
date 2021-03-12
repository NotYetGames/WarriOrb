# Steam Package & Upload

**Guides:**
1. https://partner.steamgames.com/doc/sdk/installscripts
2. https://partner.steamgames.com/doc/sdk/uploading
3. https://developer.valvesoftware.com/wiki/SteamCMD

> **Note:**
To actually upload change `preview = 0`.
That if  `preview = 1` which means that it will only test that everything is ok.

Main app build file: [`Steam/game/app_build_Warriorb_790360.vdf`](/Steam/game/app_build_Warriorb_790360.vdf)

Demo app build file: [`Steam/demo/app_build_Warriorb_demo_1193760.vdf`](/Steam/demo/app_build_Warriorb_demo_1193760.vdf)

## WarriOrb

### Expected directory structure

```sh
<WarriorbGitRepo> - The path to the Warriorb git repository

# Windows64
<WarriorbGitRepo>/Packaged/WarriOrb_Steam_Windows64/

# Linux
<WarriorbGitRepo>/Packaged/WarriOrb_Steam_Linux/
```


### Scripts

```sh
# Package ALL (Windows + Linux)
./Tools/Warriorb/Steam_PackageAll.py

# [OPTIONAL] Package Windows
./Tools/Warriorb/Steam_Package_Windows.py
# [OPTIONAL] Package Linux
./Tools/Warriorb/Steam_Package_Linux.py

# Upload
./Tools/Warriorb/Steam_Upload.py

# Sentry crash reporter
./Tools/Warriorb/Sentry_UploadDif.py --steam
```

**NOTE: to run all the commands all in one just run**
```sh
./Tools/Warriorb/Steam_All_Run.sh
```


## WarriOrb: Prologue

### Expected directory structure
```sh
<WarriorbGitRepo> - The path to the Warriorb git repository

# Windows64
<WarriorbGitRepo>/Packaged/WarriOrbPrologue_Steam_Windows64/

# Linux
<WarriorbGitRepo>/Packaged/WarriOrbPrologue_Steam_Linux/
```


### Scripts

```sh
# Package ALL (Windows + Linux)
./Tools/Warriorb/Demo_Steam_PackageAll.py

# [OPTIONAL] Package Windows
./Tools/Warriorb/Demo_Steam_PackageWindows.py
# [OPTIONAL] Package Linux
./Tools/Warriorb/Demo_Steam_PackageLinux.py

# Upload
./Tools/Warriorb/Demo_Steam_Upload.py

# Sentry crash reporter
./Tools/Warriorb/Demo_Sentry_UploadDif.py --steam
```

**NOTE: to run all the commands all in one just run**
```sh
./Tools/Warriorb/Demo_Steam_All_Run.sh
```

# Package and Upload


## WarriOrb

### Expected directory structure

```sh

```


### Scripts

```sh

```


## WarriOrb: Prologue

### Expected directory structure
```sh
<WarriorbGitRepo> - The path to the Warriorb git repository

# Windows64
<WarriorbGitRepo>/Packaged/WarriOrbPrologue_Windows64_<VERSION>_<GIT_COMMIT>/

# Linux
<WarriorbGitRepo>/Packaged/WarriOrbPrologue_Steam_Linux_<VERSION>_<GIT_COMMIT>/
```


### Scripts

```sh
# Package ALL (Windows + Linux)
./Tools/Warriorb/Demo_ItchGameJolt_PackageAll.py --unattended

# [OPTIONAL] Package Windows
# ./Tools/Warriorb/Demo_ItchGameJolt_PackageWindows.py
# [OPTIONAL] Package Linux
# ./Tools/Warriorb/Demo_ItchGameJolt_PackageLinux.py

# Upload to Itch
./Tools/Warriorb/Demo_Itch_Upload.py

# Sentry crash reporter
./Tools/Warriorb/Demo_Sentry_UploadDif.py --itch-gamejolt
```

NOTE: to run all the commands all in one just run
```sh
./Tools/Warriorb/Demo_ItchGameJolt_All_Run.sh
```

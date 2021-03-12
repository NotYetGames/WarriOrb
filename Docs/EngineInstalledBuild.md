# Engine Installed build

- https://docs.unrealengine.com/en-US/Programming/Deployment/UsinganInstalledBuild/index.html



## Make an installed build for all supported platforms


### Download source code
```sh
git clone git@gitlab.com:NotYetGames/sorb/UnrealEngine.git WarriorbUnrealEngine
cd WarriorbUnrealEngine
git checkout -b warriorb
```

### FMOD modifications

https://www.fmod.com/resources/documentation-ue4?version=2.0&page=platform-specifics.html#xbox-one


### Download XBOX XDK

https://www.microsoft.com/en-us/software-download/devcenter

### Configure
```sh
# Copy full path
cd WarriorbUnrealEngine
pwd
```

Go inside your `Warriorb/Tools/config.ini` and change the engine path to the full path of the WarriorbUnrealEngine


Download binaries Xbox and Switch by following  `DOWNLOAD_XBOX_SWITCH_BINARIES.html` an overwrite them over the engine

### Build
```sh

# Build
./Tools/UnrealEngine/MakeCompiledInstalledBuild.py --with-windows64 --with-linux --with-xbox-one --with-switch --with-full-debug-info --with-ddc --game-configurations "Debug;DebugGame;Development;Shipping"


# To clean build add
# --clean

# Move or rename
<WarriorbUnrealEngine>/LocalBuilds/Engine/Windows
```


### Clean
```sh

git clean -f -d -X -n
```

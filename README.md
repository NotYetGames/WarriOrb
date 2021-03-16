# [WarriOrb](http://warriorb.com/) source code

WarriOrb is a hardcore action platformer where you play as a demon who is trapped in an unlikely body. The game mixes the difficulty and level design of Dark Souls with an unique ball-based platforming mechanics.

![Panorama](https://user-images.githubusercontent.com/1269608/110971201-3b033900-8363-11eb-80ef-c98ff6442cf0.jpg)

## Links

### [🎆 Website](https://www.warriorb.com/)
#### [Steam](https://store.steampowered.com/app/790360/WarriOrb/)
#### [Xbox](https://www.microsoft.com/p/warriorb/9nw7s5lbk0b7)
#### [Switch](https://www.nintendo.com/games/detail/warriorb-switch/)

### [🎥 Trailer](https://www.youtube.com/watch?v=NdifbjaOQf8)

### [🔧 Unreal Plugins](https://www.unrealengine.com/marketplace/en-US/profile/Not+Yet)
#### [Dialogue System Plugin (Open Source)](https://github.com/NotYetGames/DlgSystem)

### [💬 Discord](https://discord.gg/NotYet)

## About

WarriOrb is an action platformer where you play as a mighty demon trapped in an unlikely body! Make your way through the ravaged world to regain your freedom and sanity – meeting demons, giants, mutants and all sorts of magical and crazy creatures along the way.

Key Features:
- Trapped in a Ball: Explore a tragic story of loss and desperation hidden behind a comic tale.
- Sweet Freedom: Run, jump, bounce and roll your way to freedom!
- Platforming Skills: It feels like everything is out to get you … - You got that right! Challenge your skills and reflexes against the deadliest of traps.
- Think, Think, Think: Solve challenging puzzles in between traps!
- Get Social: The world is ending! Chat with friendly and unfriendly fellows along the way about it.

![WarriOrb fithing monsters](https://user-images.githubusercontent.com/1269608/110970459-73564780-8362-11eb-8ce1-aa880362cbd9.jpg)


## Source code
- NOTE: the MIT License only applies to the code in this repository and does not include the actual commercial WarriOrb game or assets.
- This repository **only contains the code** but **NO Assets** and some missing plugins.
	- So you can't build easily from this source code.
- This source code is only for educational purposes.
- All of the [🔧 Not Yet: Unreal Plugins](https://www.unrealengine.com/marketplace/en-US/profile/Not+Yet) were developed while working on WarriOrb
	- Only the [💬 Not Yet: Dialogue System Plugin](https://github.com/NotYetGames/DlgSystem) is free and open source.

### Building

We provide these sources for our customers, and as a reference for Unreal Engine developers. **You won't be able to run the game from this repository alone**, as the game contents are not included. Building from source is only useful if you want to replace the game executable with your modifications.

#### Required dependencies

You will need the following tools to build WarriOrb from the source code:
- **[Unreal Engine 4](https://www.unrealengine.com/)** as the game engine.  You will need to sign up and download the Epic Games launcher. In the launcher library for Unreal Engine, install version **4.22**.
- [Setup Visual Studio 2017 for Unreal Engine](https://docs.unrealengine.com/en-US/Programming/Development/VisualStudioSetup/index.html) - You can download Visual Studio 2017 Community edition from [here](https://visualstudio.microsoft.com/vs/older-downloads/)
  - or after you sign in, directly from [this link](https://my.visualstudio.com/Downloads?q=visual%20studio%202017&wt.mc_id=o~msft~vscom~older-downloads) as Microsoft makes it hard to download VS 2017 (for some reason).
- [Download the **FMOD** plugin (version 1.10.19)](https://www.fmod.com/download) for UE 4.22 and copy the libs Binaries from it, inside `Plugins/FMODStudio/Binaries` directory.
  - You should copy `FMODStudio/Binaries` (after downloading and unzipping) to `Plugins/FMODStudio/Binaries`

#### Build steps


The Build command looks like this:
```
<engine_install_path>/Engine/Binaries/DotNET/UnrealBuildTool.exe <project_name><target_type> <platform> <build_type> -project=<uproject_absolute_file_path> -progress
```

To build just the Game for Windows 64 run for example:
```
"c:\dev\UE\UE_4.22\Engine\Binaries\DotNET\UnrealBuildTool.exe" Warriorb Win64 Development -project="C:\dev\WarriOrb\Warriorb.uproject" -progress
```

NOTE: You can also build the game by opening the `.sln` file in Visual Studio 2017 and building the `Development Game` target.

The resulting binaries and debug files will be generated inside `Binaries/Win64/` and can replace the equivalent files in your existing game folder.

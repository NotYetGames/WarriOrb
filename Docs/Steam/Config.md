# Config file

**Guides:**
- steam_appid.txt - https://docs.unrealengine.com/latest/INT/Programming/Online/Steam/index.html#steamappid


To configure the steam settings you need to change the config file located at [`Config/DefaultEngine.ini`](/Config/DefaultEngine.ini).

> **Note:**
That the `SteamDevAppId` is automatically set at Build time, the value is retrieved from [`Source/WarriorbBuild.json`](/Source/WarriorbBuild.json)


For more details see the Guides above.

NOTE: For non shipping builds you must edit `UE4_PROJECT_STEAMSHIPPINGID`
macro, see https://docs.unrealengine.com/en-US/Programming/Online/Steam/index.html#steamappid


Changes done with explanations:
```ini
[/Script/Engine.GameEngine]
# DefName - is the unique name of this net driver definition.
# DriverClassName - is the class name of the primary net driver.
# DriverClassNameFallBack - is the class name of the fallback net driver if the primary net driver class fails to initialize.

# Clears all the previous definitions
!NetDriverDefinitions=ClearArray

# Enabled
+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/OnlineSubsystemSteam.SteamNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")

# Disabled
+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="OnlineSubsystemUtils.IpNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")

# Use steam as the net driver
[/Script/OnlineSubsystemSteam.SteamNetDriver]
NetConnectionClassName="/Script/OnlineSubsystemSteam.SteamNetConnection"

# Configure the online stuff
[OnlineSubsystem]

# Use nothing (LAN)
DefaultPlatformService=Null

# Use steam
DefaultPlatformService=Steam

# Default value is 50 ms
PollingIntervalInMs=50

# Enable/Disable Foice
bHasVoiceEnabled=False

# Override the build ID for the online subsystem
bUseBuildIdOverride=false

# Int : the ovverride
BuildIdOverride=0

[OnlineSubsystemSteam]
bEnabled=True

# 480 is SpaceWar
# This is only used in NON SHIPPING builds
SteamDevAppId=480
GameServerQueryPort=27015

# Seems to be only useful for server advertising.
# NOTE: Only useful in multiplayer games.
GameVersion=1.0.0.0

# Relaunch the game with steam if necessary
bRelaunchInSteam=True

# VAC cheat protection
# NOTE: Only useful in multiplayer games.
# See: https://partner.steamgames.com/doc/api/steam_gameserver
# True - eServerModeAuthenticationAndSecure
# False - eServerModeAuthentication
bVACEnabled=False

# Allow P2P connections to fall back to being relayed through the Steam servers if a direct connection
# or NAT-traversal cannot be established. Only applies to connections created after setting this value,
# or to existing connections that need to automatically reconnect after this value is set.
# NOTE: Only useful in multiplayer games.
bAllowP2PPacketRelay=true
P2PConnectionTimeout=90
```

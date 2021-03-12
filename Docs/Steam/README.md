# Steam

Guides:
- https://docs.unrealengine.com/latest/INT/Programming/Online/Steam/
- https://wiki.unrealengine.com/Steam,_Using_the_Steam_SDK_During_Development

## Disable

### At runtime

You can disable steam at runtime (in non shipping builds) by using the `-nosteam` argument when running.

### At build time

Edit the key `WARRIORB_WITH_STEAM` inside [`Source/WarriorbBuild.json`](/Source/WarriorbBuild.json) and rebuilt the project.


## [Package & Upload](./PackageAndUpload.md)

## [Achievements](./Achievements.md)

## [Local Content Server](./LocalContentServer.md)

## [Config file]((./Config.md))

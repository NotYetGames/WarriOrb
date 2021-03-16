# GameAnalytics

Modified CPP SDK: https://github.com/vampy/GA-SDK-CPP

Modified UNREAL SDK: https://github.com/vampy/GA-SDK-UNREAL


Copied manually from https://github.com/GameAnalytics/GA-SDK-UNREAL
because it is the latest version and also on linux we do not have a marketplace.

## Documentation
1. https://gameanalytics.com/docs/unreal4-sdk
2. https://gameanalytics.com/docs/


## Build

NOTE: use the modified CPP SDK, otherwise the build/link will fail.

Windows:
```sh
./build.sh -n -t win64-vc140-static
./build.sh -n -t win32-vc140-static

OR

./build_win_py2.sh -n -t win64-vc140-static
./build_win_py2.sh -n -t win32-vc140-static
```

Linux:
```sh
./build.sh -n -t linux-x64-clang-static
```

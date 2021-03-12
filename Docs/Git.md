# GIT install & update

## Install
1. Make sure you have [Unreal engine](https://www.unrealengine.com) installed.

2. Install a [git client](https://wiki.unrealengine.com/Git_source_control_(Tutorial)#Recommended_Git_GUI.27s) and clone the repository.

3. Clone from the GUI, remember to also initialize the submodules or use the command line to clone the repo properly:
```sh
# Or for SSH git@gitlab.com:NotYetGames/sorb/code.git
git clone --recursive -j2 https://gitlab.com/NotYetGames/sorb/code2.git
```
```sh
# If you cloned without the --recursive flag, run the following command
git submodule update --init --recursive
```

4. After the repository is cloned, right click on the `Warriorb.uproject` -> `Generate Visual Studio project files`.

   Or just run `./Tools/GenerateProjectFiles.sh` (read the [`./Tools/README.md`](Tools/README.md) first)


## Update
Usually `git pull` works but if you also want to update the submodules you must run:
```sh
git submodule foreach git pull origin
```

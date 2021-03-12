#!/bin/sh
UE4_TRUE_SCRIPT_NAME=$(echo \"$0\" | xargs readlink -f)
UE4_PROJECT_ROOT=$(dirname "$UE4_TRUE_SCRIPT_NAME")
chmod +x "$UE4_PROJECT_ROOT/Warriorb/Binaries/Linux/Warriorb"

echo ""
echo "$@"
echo ""

# Bug in linux, if this is set the gamepad will not work sometimes
# Because Steam runtime + UE 4.22 combination
unset SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT

# printenv > steam.txt

"$UE4_PROJECT_ROOT/Warriorb/Binaries/Linux/Warriorb" Warriorb "$@"


# #!/usr/bin/env bash
# UE4_TRUE_SCRIPT_NAME=$(echo \"$0\" | xargs readlink -f)
# UE4_PROJECT_ROOT=$(dirname "$UE4_TRUE_SCRIPT_NAME")
# chmod +x "$UE4_PROJECT_ROOT/Warriorb/Binaries/Linux/Warriorb"

# # OLD: Modified version becaus GA tries to write ga_log.txt but it crashes the editor
# # New verion tries to fix some bug in UE?
# pushd "$UE4_PROJECT_ROOT/Warriorb/Binaries/Linux/"
# LC_ALL=C "./Warriorb" Warriorb "$@"
# popd
